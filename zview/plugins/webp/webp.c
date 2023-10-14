/*
 * webp.c - common code for SLB and LDG plugins
 *
 * Copyright (C) 2023 Thorsten Otto
 *
 * For conditions of distribution and use, see copyright file.
 */

#include "plugin.h"
#include "zvplugin.h"
#include <webp/decode.h>
#include <webp/encode.h>
#include <webp/mux_types.h>
#include "zvwebp.h"

#define VERSION 0x100
#define NAME    "Image format for the Web"
#define AUTHOR  "Thorsten Otto"
#define DATE     __DATE__ " " __TIME__
#define MISCINFO "Using libwep version " WEBP_VERSION_STR

#define NF_DEBUG 0

#define MKFOURCC(a, b, c, d) ((a) | (b) << 8 | (c) << 16 | (uint32_t)(d) << 24)

/* Mux related constants. */
#define TAG_SIZE           4     /* Size of a chunk tag (e.g. "VP8L"). */
#define CHUNK_SIZE_BYTES   4     /* Size needed to store chunk's size. */
#define CHUNK_HEADER_SIZE  8     /* Size of a chunk header. */
#define RIFF_HEADER_SIZE   12    /* Size of the RIFF header ("RIFFnnnnWEBP"). */
#define ANMF_CHUNK_SIZE    16    /* Size of an ANMF chunk. */
#define ANIM_CHUNK_SIZE    6     /* Size of an ANIM chunk. */
#define VP8X_CHUNK_SIZE    10    /* Size of a VP8X chunk. */


#define LOG_ERROR(MESSAGE) nf_debugprint((MESSAGE))
#define LOG_WARN(MESSAGE) nf_debugprint((MESSAGE))

typedef enum
{
	WEBP_INFO_OK = 0,
	WEBP_INFO_TRUNCATED_DATA,
	WEBP_INFO_PARSE_ERROR,
	WEBP_INFO_INVALID_PARAM,
	WEBP_INFO_BITSTREAM_ERROR,
	WEBP_INFO_MISSING_DATA,
	WEBP_INFO_INVALID_COMMAND
} WebPInfoStatus;

typedef enum ChunkID
{
	CHUNK_VP8,
	CHUNK_VP8L,
	CHUNK_VP8X,
	CHUNK_ALPHA,
	CHUNK_ANIM,
	CHUNK_ANMF,
	CHUNK_ICCP,
	CHUNK_EXIF,
	CHUNK_XMP,
	CHUNK_UNKNOWN,
	CHUNK_TYPES
} ChunkID;

typedef struct
{
	size_t pos;
	size_t size;
	const uint8_t *buf_;
} MemBuffer;

typedef struct
{
	size_t offset_;
	size_t size_;
	const uint8_t *payload_;
	ChunkID id_;
} ChunkData;



#if NF_DEBUG

#define DEBUG_PREFIX "WebP: "

#include <mint/arch/nf_ops.h>

#ifdef PLUGIN_SLB

static long nfid_stderr;

#pragma GCC optimize "-fomit-frame-pointer"

static long __attribute__((noinline)) __CDECL _nf_get_id(const char *feature_name)
{
	register long ret __asm__ ("d0");
	(void)(feature_name);
	__asm__ volatile(
		"\t.word 0x7300\n"
	: "=g"(ret)  /* outputs */
	: /* inputs  */
	: __CLOBBER_RETURN("d0") "d1", "cc" AND_MEMORY /* clobbered regs */
	);
	return ret;
}


static long __attribute__((noinline)) __CDECL _nf_call(long id, ...)
{
	register long ret __asm__ ("d0");
	(void)(id);
	__asm__ volatile(
		"\t.word 0x7301\n"
	: "=g"(ret)  /* outputs */
	: /* inputs  */
	: __CLOBBER_RETURN("d0") "d1", "cc" AND_MEMORY /* clobbered regs */
	);
	return ret;
}

/*
 * call the functions indirectly, so the unused parameters above
 * are not optimized away
 */
static struct {
	long __CDECL (*nf_get_id)(const char *feature_name);
	long __CDECL (*nf_call)(long id, ...);
} ops = {
	_nf_get_id,
	_nf_call
};


__attribute__((format(__printf__, 1, 0)))
int nf_debugvprintf(const char *format, va_list args)
{
	static char buf[2048];
	int ret;

	if (nfid_stderr == 0)
		nfid_stderr = ops.nf_get_id("NF_STDERR");
	ret = vsnprintf(buf, sizeof(buf), format, args);
	ret = (int)ops.nf_call(nfid_stderr | 0, (uint32_t)buf);
	return ret;
}

__attribute__((format(__printf__, 1, 2)))
int nf_debugprintf(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = nf_debugvprintf(format, args);
	va_end(args);
	return ret;
}

#endif

#define nf_debugprint(x) nf_debugprintf x

#else

#define nf_debugprint(x)

#endif


uint16_t compression_level = 6;
int quality = 75;

#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	nf_debugprint((DEBUG_PREFIX "get_option: %ld\n", (long) which));
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE | CAN_ENCODE;
	case OPTION_EXTENSIONS:
		return (long) ("WEB\0" "WEBP\0");

	case OPTION_QUALITY:
		return quality;
	case OPTION_COMPRESSION:
		return compression_level;

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
	case INFO_MISC:
		return (long)MISCINFO;
	}
	return -ENOSYS;
}

long __CDECL set_option(zv_int_t which, zv_int_t value)
{
	nf_debugprint((DEBUG_PREFIX "set_option: %ld\n", (long) which));
	switch (which)
	{
	case OPTION_CAPABILITIES:
	case OPTION_EXTENSIONS:
		return -EACCES;
	case OPTION_QUALITY:
		if (value < 0 || value > 100)
			return -ERANGE;
		quality = value;
		return value;
	case OPTION_COMPRESSION:
		compression_level = value;
		return value;
	}
	return -ENOSYS;
}

#ifdef WEBP_SLB

static long init_webp_slb(void)
{
	struct _zview_plugin_funcs *funcs;
	SLB *slb;
	long ret;

	funcs = get_slb_funcs();
	slb = get_slb_funcs()->p_slb_get(LIB_WEBP);
	if (slb->handle == 0)
	{
		if ((ret = funcs->p_slb_open(LIB_WEBP)) < 0)
			return ret;
	}
	return 0;
}
#endif

#endif


#define alpha_composite(composite, fg, alpha, bg) {									\
    uint16_t temp = (( uint16_t)( fg) * ( uint16_t)( alpha) +						\
                   ( uint16_t)( bg) * ( uint16_t)(255 - ( uint16_t)( alpha)) + ( uint16_t)128);	\
    ( composite) = ( uint8_t)(( temp + ( temp >> 8)) >> 8);								\
}

typedef struct _mywebp_info {
	VP8StatusCode status;
	WebPData input_data;
	FILE *webp_file;
	uint32_t feature_flags;
	int has_alpha;
	int has_animation;
	int lossless;
	int32_t canvas_width;
	int32_t canvas_height;
	int loop_count;
	uint32_t bgcolor;
	int32_t num_frames;
	uint8_t *bmap;
	uint8_t channels;
	size_t stride_width;
	size_t row_width;
	/* used for parsing */
	int chunk_counts[CHUNK_TYPES];
	int anmf_subchunk_counts[3];		/* 0 VP8; 1 VP8L; 2 ALPH. */
	int32_t frame_width, frame_height;
	size_t anim_frame_data_size;
	int is_processing_anim_frame;
	int seen_alpha_subchunk;
	int seen_image_subchunk;
} WebPInfo;

static uint32_t filesize(int16_t fd)
{
	uint32_t size;

	size = Fseek(0, fd, SEEK_END);
	Fseek(0, fd, SEEK_SET);
	return size;
}

/*==================================================================================*
 * WebP chunk parsing
 *==================================================================================*/

static const uint32_t kWebPChunkTags[CHUNK_UNKNOWN] = {
	MKFOURCC('V', 'P', '8', ' '),
	MKFOURCC('V', 'P', '8', 'L'),
	MKFOURCC('V', 'P', '8', 'X'),
	MKFOURCC('A', 'L', 'P', 'H'),
	MKFOURCC('A', 'N', 'I', 'M'),
	MKFOURCC('A', 'N', 'M', 'F'),
	MKFOURCC('I', 'C', 'C', 'P'),
	MKFOURCC('E', 'X', 'I', 'F'),
	MKFOURCC('X', 'M', 'P', ' '),
};

static int GetLE16(const uint8_t *data)
{
	return (data[0] << 0) | (data[1] << 8);
}

static int GetLE24(const uint8_t *data)
{
	return GetLE16(data) | (data[2] << 16);
}

static uint32_t GetLE32(const uint8_t *data)
{
	return GetLE16(data) | ((uint32_t) GetLE16(data + 2) << 16);
}

static int ReadLE16(const uint8_t **data)
{
	const int val = GetLE16(*data);

	*data += 2;
	return val;
}

static int ReadLE24(const uint8_t **data)
{
	const int val = GetLE24(*data);

	*data += 3;
	return val;
}

static uint32_t ReadLE32(const uint8_t **data)
{
	const uint32_t val = GetLE32(*data);

	*data += 4;
	return val;
}

static void InitMemBuffer(MemBuffer *mem, const WebPData *webp_data)
{
	mem->buf_ = webp_data->bytes;
	mem->pos = 0;
	mem->size = webp_data->size;
}

static size_t MemDataSize(const MemBuffer *mem)
{
	return (mem->size - mem->pos);
}

static const uint8_t *GetBuffer(MemBuffer *mem)
{
	return mem->buf_ + mem->pos;
}

static void Skip(MemBuffer *mem, size_t size)
{
	mem->pos += size;
}

static uint32_t ReadMemBufLE32(MemBuffer *mem)
{
	const uint8_t *data = mem->buf_ + mem->pos;
	const uint32_t val = GetLE32(data);

	Skip(mem, 4);
	return val;
}

static WebPInfoStatus ParseRIFFHeader(WebPInfo *webp_info, MemBuffer *mem)
{
	const size_t min_size = RIFF_HEADER_SIZE + CHUNK_HEADER_SIZE;
	size_t riff_size;

	if (MemDataSize(mem) < min_size)
	{
		LOG_ERROR("Truncated data detected when parsing RIFF header.");
		return WEBP_INFO_TRUNCATED_DATA;
	}
	if (memcmp(GetBuffer(mem), "RIFF", CHUNK_SIZE_BYTES) ||
		memcmp(GetBuffer(mem) + CHUNK_HEADER_SIZE, "WEBP", CHUNK_SIZE_BYTES))
	{
		LOG_ERROR("Corrupted RIFF header.");
		return WEBP_INFO_PARSE_ERROR;
	}
	riff_size = GetLE32(GetBuffer(mem) + TAG_SIZE);
	if (riff_size < CHUNK_HEADER_SIZE)
	{
		LOG_ERROR("RIFF size is too small.");
		return WEBP_INFO_PARSE_ERROR;
	}
	riff_size += CHUNK_HEADER_SIZE;
	if (riff_size < mem->size)
	{
		LOG_WARN("RIFF size is smaller than the file size.");
		mem->size = riff_size;
	} else if (riff_size > mem->size)
	{
		LOG_ERROR("Truncated data detected when parsing RIFF payload.");
		return WEBP_INFO_TRUNCATED_DATA;
	}
	Skip(mem, RIFF_HEADER_SIZE);
	return WEBP_INFO_OK;
}


static WebPInfoStatus ParseChunk(const WebPInfo *webp_info, MemBuffer *mem, ChunkData *chunk_data)
{
	memset(chunk_data, 0, sizeof(*chunk_data));
	if (MemDataSize(mem) < CHUNK_HEADER_SIZE)
	{
		LOG_ERROR("Truncated data detected when parsing chunk header.");
		return WEBP_INFO_TRUNCATED_DATA;
	} else
	{
		const size_t chunk_start_offset = mem->pos;
		const uint32_t fourcc = ReadMemBufLE32(mem);
		const uint32_t payload_size = ReadMemBufLE32(mem);
		const uint32_t payload_size_padded = payload_size + (payload_size & 1);
		const size_t chunk_size = CHUNK_HEADER_SIZE + payload_size_padded;
		int i;

		if (payload_size_padded > MemDataSize(mem))
		{
			LOG_ERROR("Truncated data detected when parsing chunk payload.");
			return WEBP_INFO_TRUNCATED_DATA;
		}
		for (i = 0; i < CHUNK_UNKNOWN; ++i)
		{
			if (kWebPChunkTags[i] == fourcc)
				break;
		}
		chunk_data->offset_ = chunk_start_offset;
		chunk_data->size_ = chunk_size;
		chunk_data->id_ = (ChunkID) i;
		chunk_data->payload_ = GetBuffer(mem);
		if (chunk_data->id_ == CHUNK_ANMF)
		{
			if (payload_size != payload_size_padded)
			{
				LOG_ERROR("ANMF chunk size should always be even.");
				return WEBP_INFO_PARSE_ERROR;
			}
			/* There are sub-chunks to be parsed in an ANMF chunk. */
			Skip(mem, ANMF_CHUNK_SIZE);
		} else
		{
			Skip(mem, payload_size_padded);
		}
		return WEBP_INFO_OK;
	}
}

/* ----------------------------------------------------------------------------- */
/* Chunk analysis. */

static WebPInfoStatus ProcessVP8XChunk(const ChunkData *chunk_data, WebPInfo *webp_info)
{
	const uint8_t *data = chunk_data->payload_;

	if (webp_info->chunk_counts[CHUNK_VP8] ||
		webp_info->chunk_counts[CHUNK_VP8L] ||
		webp_info->chunk_counts[CHUNK_VP8X])
	{
		LOG_ERROR("Already seen a VP8/VP8L/VP8X chunk when parsing VP8X chunk.");
		return WEBP_INFO_PARSE_ERROR;
	}
	if (chunk_data->size_ != VP8X_CHUNK_SIZE + CHUNK_HEADER_SIZE)
	{
		LOG_ERROR("Corrupted VP8X chunk.");
		return WEBP_INFO_PARSE_ERROR;
	}
	++webp_info->chunk_counts[CHUNK_VP8X];
	webp_info->feature_flags = *data;
	data += 4;
	webp_info->canvas_width = 1 + ReadLE24(&data);
	webp_info->canvas_height = 1 + ReadLE24(&data);
	return WEBP_INFO_OK;
}


static WebPInfoStatus ProcessANIMChunk(const ChunkData *chunk_data, WebPInfo *webp_info)
{
	const uint8_t *data = chunk_data->payload_;

	if (!webp_info->chunk_counts[CHUNK_VP8X])
	{
		LOG_ERROR("ANIM chunk detected before VP8X chunk.");
		return WEBP_INFO_PARSE_ERROR;
	}
	if (chunk_data->size_ != ANIM_CHUNK_SIZE + CHUNK_HEADER_SIZE)
	{
		LOG_ERROR("Corrupted ANIM chunk.");
		return WEBP_INFO_PARSE_ERROR;
	}
	webp_info->bgcolor = ReadLE32(&data);
	webp_info->loop_count = ReadLE16(&data);
	++webp_info->chunk_counts[CHUNK_ANIM];
	return WEBP_INFO_OK;
}


static WebPInfoStatus ProcessANMFChunk(const ChunkData *chunk_data, WebPInfo *webp_info)
{
	const uint8_t *data = chunk_data->payload_;
	int offset_x, offset_y;
	int width, height;
	int duration;
	int blend;
	int dispose;
	int temp;

	if (webp_info->is_processing_anim_frame)
	{
		LOG_ERROR("ANMF chunk detected within another ANMF chunk.");
		return WEBP_INFO_PARSE_ERROR;
	}
	if (!webp_info->chunk_counts[CHUNK_ANIM])
	{
		LOG_ERROR("ANMF chunk detected before ANIM chunk.");
		return WEBP_INFO_PARSE_ERROR;
	}
	if (chunk_data->size_ <= CHUNK_HEADER_SIZE + ANMF_CHUNK_SIZE)
	{
		LOG_ERROR("Truncated data detected when parsing ANMF chunk.");
		return WEBP_INFO_TRUNCATED_DATA;
	}
	offset_x = 2 * ReadLE24(&data);
	offset_y = 2 * ReadLE24(&data);
	width = 1 + ReadLE24(&data);
	height = 1 + ReadLE24(&data);
	duration = ReadLE24(&data);
	temp = *data;
	dispose = temp & 1;
	blend = (temp >> 1) & 1;
	(void) offset_x;
	(void) offset_y;
	(void) dispose;
	(void) blend;
	(void) duration;
	++webp_info->chunk_counts[CHUNK_ANMF];
	webp_info->has_animation = TRUE;
	webp_info->feature_flags |= ANIMATION_FLAG;
	webp_info->is_processing_anim_frame = 1;
	webp_info->seen_alpha_subchunk = 0;
	webp_info->seen_image_subchunk = 0;
	webp_info->frame_width = width;
	webp_info->frame_height = height;
	webp_info->anim_frame_data_size = chunk_data->size_ - CHUNK_HEADER_SIZE - ANMF_CHUNK_SIZE;
	return WEBP_INFO_OK;
}


static WebPInfoStatus ProcessImageChunk(const ChunkData *chunk_data, WebPInfo *webp_info)
{
	const uint8_t *data = chunk_data->payload_ - CHUNK_HEADER_SIZE;
	WebPBitstreamFeatures features;
	const VP8StatusCode vp8_status = WebPGetFeatures(data, chunk_data->size_, &features);

	if (vp8_status != VP8_STATUS_OK)
	{
		LOG_ERROR("VP8/VP8L bitstream error.");
		return WEBP_INFO_BITSTREAM_ERROR;
	}
	if (features.format == 2)
		webp_info->lossless = TRUE;
	if (features.has_alpha)
	{
		webp_info->has_alpha = TRUE;
		webp_info->feature_flags |= ALPHA_FLAG;
	}
	if (features.has_animation)
	{
		webp_info->has_animation = TRUE;
	}
	webp_info->frame_width = features.width;
	webp_info->frame_height = features.height;
	if (webp_info->chunk_counts[CHUNK_VP8X] == 0 &&
		webp_info->frame_width > 0 &&
		webp_info->frame_height > 0)
	{
		webp_info->canvas_width = webp_info->frame_width;
		webp_info->canvas_height = webp_info->frame_height;
	}
	if (webp_info->is_processing_anim_frame)
	{
		++webp_info->anmf_subchunk_counts[chunk_data->id_ == CHUNK_VP8 ? 0 : 1];
		if (chunk_data->id_ == CHUNK_VP8L && webp_info->seen_alpha_subchunk)
		{
			LOG_ERROR("Both VP8L and ALPH sub-chunks are present in an ANMF chunk.");
			return WEBP_INFO_PARSE_ERROR;
		}
		if (webp_info->frame_width != features.width || webp_info->frame_height != features.height)
		{
			LOG_ERROR("Frame size in VP8/VP8L sub-chunk differs from ANMF header.");
			return WEBP_INFO_PARSE_ERROR;
		}
		if (webp_info->seen_image_subchunk)
		{
			LOG_ERROR("Consecutive VP8/VP8L sub-chunks in an ANMF chunk.");
			return WEBP_INFO_PARSE_ERROR;
		}
		webp_info->seen_image_subchunk = 1;
	} else
	{
		if (webp_info->chunk_counts[CHUNK_VP8] || webp_info->chunk_counts[CHUNK_VP8L])
		{
			LOG_ERROR("Multiple VP8/VP8L chunks detected.");
			return WEBP_INFO_PARSE_ERROR;
		}
		if (chunk_data->id_ == CHUNK_VP8L && webp_info->chunk_counts[CHUNK_ALPHA])
		{
			LOG_WARN("Both VP8L and ALPH chunks are detected.");
		}
		if (webp_info->chunk_counts[CHUNK_ANIM] || webp_info->chunk_counts[CHUNK_ANMF])
		{
			LOG_ERROR("VP8/VP8L chunk and ANIM/ANMF chunk are both detected.");
			return WEBP_INFO_PARSE_ERROR;
		}
		++webp_info->chunk_counts[chunk_data->id_];
	}
	++webp_info->num_frames;
	return WEBP_INFO_OK;
}


static WebPInfoStatus ProcessALPHChunk(const ChunkData *chunk_data, WebPInfo *webp_info)
{
	if (webp_info->is_processing_anim_frame)
	{
		++webp_info->anmf_subchunk_counts[2];
		if (webp_info->seen_alpha_subchunk)
		{
			LOG_ERROR("Consecutive ALPH sub-chunks in an ANMF chunk.");
			return WEBP_INFO_PARSE_ERROR;
		}
		webp_info->seen_alpha_subchunk = 1;

		if (webp_info->seen_image_subchunk)
		{
			LOG_ERROR("ALPHA sub-chunk detected after VP8 sub-chunk " "in an ANMF chunk.");
			return WEBP_INFO_PARSE_ERROR;
		}
	} else
	{
		if (webp_info->chunk_counts[CHUNK_ANIM] || webp_info->chunk_counts[CHUNK_ANMF])
		{
			LOG_ERROR("ALPHA chunk and ANIM/ANMF chunk are both detected.");
			return WEBP_INFO_PARSE_ERROR;
		}
		if (!webp_info->chunk_counts[CHUNK_VP8X])
		{
			LOG_ERROR("ALPHA chunk detected before VP8X chunk.");
			return WEBP_INFO_PARSE_ERROR;
		}
		if (webp_info->chunk_counts[CHUNK_VP8])
		{
			LOG_ERROR("ALPHA chunk detected after VP8 chunk.");
			return WEBP_INFO_PARSE_ERROR;
		}
		if (webp_info->chunk_counts[CHUNK_ALPHA])
		{
			LOG_ERROR("Multiple ALPHA chunks detected.");
			return WEBP_INFO_PARSE_ERROR;
		}
		++webp_info->chunk_counts[CHUNK_ALPHA];
	}
	webp_info->has_alpha = TRUE;
	webp_info->feature_flags |= ALPHA_FLAG;
	return WEBP_INFO_OK;
}


static WebPInfoStatus ProcessICCPChunk(const ChunkData *chunk_data, WebPInfo *webp_info)
{
	(void) chunk_data;
	if (!webp_info->chunk_counts[CHUNK_VP8X])
	{
		LOG_ERROR("ICCP chunk detected before VP8X chunk.");
		return WEBP_INFO_PARSE_ERROR;
	}
	if (webp_info->chunk_counts[CHUNK_VP8] ||
		webp_info->chunk_counts[CHUNK_VP8L] ||
		webp_info->chunk_counts[CHUNK_ANIM])
	{
		LOG_ERROR("ICCP chunk detected after image data.");
		return WEBP_INFO_PARSE_ERROR;
	}
	++webp_info->chunk_counts[CHUNK_ICCP];
	return WEBP_INFO_OK;
}


static WebPInfoStatus ProcessChunk(const ChunkData *chunk_data, WebPInfo *webp_info)
{
	WebPInfoStatus status = WEBP_INFO_OK;
	ChunkID id = chunk_data->id_;

	switch (id)
	{
	case CHUNK_VP8:
	case CHUNK_VP8L:
		status = ProcessImageChunk(chunk_data, webp_info);
		break;
	case CHUNK_VP8X:
		status = ProcessVP8XChunk(chunk_data, webp_info);
		break;
	case CHUNK_ALPHA:
		status = ProcessALPHChunk(chunk_data, webp_info);
		break;
	case CHUNK_ANIM:
		status = ProcessANIMChunk(chunk_data, webp_info);
		break;
	case CHUNK_ANMF:
		status = ProcessANMFChunk(chunk_data, webp_info);
		break;
	case CHUNK_ICCP:
		status = ProcessICCPChunk(chunk_data, webp_info);
		break;
	case CHUNK_EXIF:
	case CHUNK_XMP:
		++webp_info->chunk_counts[id];
		break;
	case CHUNK_UNKNOWN:
	default:
		break;
	}
	if (webp_info->is_processing_anim_frame && id != CHUNK_ANMF)
	{
		if (webp_info->anim_frame_data_size == chunk_data->size_)
		{
			if (!webp_info->seen_image_subchunk)
			{
				LOG_ERROR("No VP8/VP8L chunk detected in an ANMF chunk.");
				return WEBP_INFO_PARSE_ERROR;
			}
			webp_info->is_processing_anim_frame = 0;
		} else if (webp_info->anim_frame_data_size > chunk_data->size_)
		{
			webp_info->anim_frame_data_size -= chunk_data->size_;
		} else
		{
			LOG_ERROR("Truncated data detected when parsing ANMF chunk.");
			return WEBP_INFO_TRUNCATED_DATA;
		}
	}
	return status;
}


static WebPInfoStatus Validate(WebPInfo *webp_info)
{
	if (webp_info->num_frames < 1)
	{
		LOG_ERROR("No image/frame detected.");
		return WEBP_INFO_MISSING_DATA;
	}
	if (webp_info->chunk_counts[CHUNK_VP8X])
	{
		const int iccp = (webp_info->feature_flags & ICCP_FLAG) != 0;
		const int exif = (webp_info->feature_flags & EXIF_FLAG) != 0;
		const int xmp = (webp_info->feature_flags & XMP_FLAG) != 0;
		const int animation = (webp_info->feature_flags & ANIMATION_FLAG) != 0;
		const int alpha = (webp_info->feature_flags & ALPHA_FLAG) != 0;

		if (!alpha && webp_info->has_alpha)
		{
			LOG_ERROR("Unexpected alpha data detected.");
			return WEBP_INFO_PARSE_ERROR;
		}
		if (alpha && !webp_info->has_alpha)
		{
			LOG_WARN("Alpha flag is set with no alpha data present.");
		}
		if (iccp && !webp_info->chunk_counts[CHUNK_ICCP])
		{
			LOG_ERROR("Missing ICCP chunk.");
			return WEBP_INFO_MISSING_DATA;
		}
		if (exif && !webp_info->chunk_counts[CHUNK_EXIF])
		{
			LOG_ERROR("Missing EXIF chunk.");
			return WEBP_INFO_MISSING_DATA;
		}
		if (xmp && !webp_info->chunk_counts[CHUNK_XMP])
		{
			LOG_ERROR("Missing XMP chunk.");
			return WEBP_INFO_MISSING_DATA;
		}
		if (!iccp && webp_info->chunk_counts[CHUNK_ICCP])
		{
			LOG_ERROR("Unexpected ICCP chunk detected.");
			return WEBP_INFO_PARSE_ERROR;
		}
		if (!exif && webp_info->chunk_counts[CHUNK_EXIF])
		{
			LOG_ERROR("Unexpected EXIF chunk detected.");
			return WEBP_INFO_PARSE_ERROR;
		}
		if (!xmp && webp_info->chunk_counts[CHUNK_XMP])
		{
			LOG_ERROR("Unexpected XMP chunk detected.");
			return WEBP_INFO_PARSE_ERROR;
		}
		/* Incomplete animation frame. */
		if (webp_info->is_processing_anim_frame)
			return WEBP_INFO_MISSING_DATA;
		if (!animation && webp_info->num_frames > 1)
		{
			LOG_ERROR("More than 1 frame detected in non-animation file.");
			return WEBP_INFO_PARSE_ERROR;
		}
		if (animation && (!webp_info->chunk_counts[CHUNK_ANIM] || !webp_info->chunk_counts[CHUNK_ANMF]))
		{
			LOG_ERROR("No ANIM/ANMF chunk detected in animation file.");
			return WEBP_INFO_PARSE_ERROR;
		}
	}
	return WEBP_INFO_OK;
}


static WebPInfoStatus AnalyzeWebP(WebPInfo *webp_info, const WebPData *webp_data)
{
	ChunkData chunk_data;
	MemBuffer mem_buffer;
	WebPInfoStatus webp_info_status = WEBP_INFO_OK;

	InitMemBuffer(&mem_buffer, webp_data);
	webp_info_status = ParseRIFFHeader(webp_info, &mem_buffer);
	if (webp_info_status != WEBP_INFO_OK)
		return webp_info_status;

	/*  Loop through all the chunks. Terminate immediately in case of error. */
	while (webp_info_status == WEBP_INFO_OK && MemDataSize(&mem_buffer) > 0)
	{
		webp_info_status = ParseChunk(webp_info, &mem_buffer, &chunk_data);
		if (webp_info_status != WEBP_INFO_OK)
			return webp_info_status;
		webp_info_status = ProcessChunk(&chunk_data, webp_info);
	}
	if (webp_info_status != WEBP_INFO_OK)
		return webp_info_status;

	/*  Final check. */
	webp_info_status = Validate(webp_info);

	return webp_info_status;
}

/*==================================================================================*
 * boolean __CDECL reader_init:														*
 *		Open the file "name", fit the "info" struct. ( see zview.h) and make others	*
 *		things needed by the decoder.												*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		name		->	The file to open.											*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	WebPInfo *myinfo;
	uint32_t file_length;
	int16_t handle;
	size_t bitmap_size;

	nf_debugprint((DEBUG_PREFIX "reader_init: %p %s\n", (void *)info, name));

#ifdef PLUGIN_SLB
#ifdef WEBP_SLB
	if (init_webp_slb() < 0)
	{
		nf_debugprint((DEBUG_PREFIX "init_webp_slb() failed\n"));
		return FALSE;
	}
#endif
#endif

	if ((handle = (int16_t) Fopen(name, 0)) < 0)
	{
		nf_debugprint((DEBUG_PREFIX "fopen() failed\n"));
		return FALSE;
	}

	file_length = filesize(handle);

	if ((myinfo = malloc(sizeof(*myinfo) + file_length + 1)) == NULL)
	{
		nf_debugprint((DEBUG_PREFIX "malloc() failed\n"));
		Fclose(handle);
		return FALSE;
	}
	memset(myinfo, 0, sizeof(*myinfo));
	myinfo->input_data.bytes = (uint8_t *)myinfo + sizeof(*myinfo);
	myinfo->input_data.size = file_length;
	info->_priv_ptr = myinfo;

	if ((uint32_t) Fread(handle, file_length, myinfo->input_data.bytes) != file_length)
	{
		nf_debugprint((DEBUG_PREFIX "fread() failed\n"));
		Fclose(handle);
		reader_quit(info);
		return FALSE;
	}
	Fclose(handle);
	if (AnalyzeWebP(myinfo, &myinfo->input_data) != WEBP_INFO_OK)
	{
		nf_debugprint((DEBUG_PREFIX "AnalyzeWebP() failed\n"));
		reader_quit(info);
		return FALSE;
	}

	/* check for resonable limits */
	if (myinfo->canvas_width <= 0 ||
		myinfo->canvas_width >= 32000 ||
		myinfo->canvas_height <= 0 ||
		myinfo->canvas_height >= 32000)
	{
		nf_debugprint((DEBUG_PREFIX "image too large: %ldx%ld\n", (long)myinfo->canvas_width, (long)myinfo->canvas_height));
		reader_quit(info);
	}
	nf_debugprint((DEBUG_PREFIX "width: %u\n", myinfo->canvas_width));
	nf_debugprint((DEBUG_PREFIX "height: %u\n", myinfo->canvas_height));
	nf_debugprint((DEBUG_PREFIX "frames: %u\n", myinfo->num_frames));

	myinfo->channels = myinfo->has_alpha ? 4 : 3;
	myinfo->stride_width = myinfo->canvas_width * myinfo->channels;
	myinfo->row_width = myinfo->canvas_width * 3;
	bitmap_size = myinfo->stride_width * myinfo->canvas_height;
	myinfo->bmap = WebPMalloc(bitmap_size);
	if (myinfo->bmap == NULL)
	{
		nf_debugprint((DEBUG_PREFIX "malloc() failed\n"));
		reader_quit(info);
		return FALSE;
	}
	
	strcpy(info->info, "WebP image format");
	info->width = myinfo->canvas_width;
	info->height = myinfo->canvas_height;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->components = myinfo->channels > 3 ? 3 : myinfo->channels;
	info->planes = myinfo->channels * 8;
	info->colors = 1L << MIN(info->planes, 24);
	info->page = 1;
	info->delay = 0;
	info->num_comments = 0;
	info->max_comments_length = 0;
	info->indexed_color = FALSE;
	info->orientation = UP_TO_DOWN;

	/* Attention: compression field is only 5 chars */
	strcpy(info->compression, "Unkn");
	if (myinfo->chunk_counts[CHUNK_VP8L] > 0)
	{
		strcpy(info->compression, "VP8L");
	} else if (myinfo->chunk_counts[CHUNK_VP8] > 0)
	{
		strcpy(info->compression, "VP8");
	}
	if (myinfo->lossless)
		strcat(info->info, " (lossless)");

	if (myinfo->channels == 4)
		WebPDecodeRGBAInto(myinfo->input_data.bytes, myinfo->input_data.size, myinfo->bmap, bitmap_size, myinfo->stride_width);
	else
		WebPDecodeRGBInto(myinfo->input_data.bytes, myinfo->input_data.size, myinfo->bmap, bitmap_size, myinfo->row_width);

	return TRUE;
}


/*==================================================================================*
 * boolean __CDECL reader_get_txt													*
 *		This function , like other function mus be always present.					*
 *		It fills txtdata struct. with the text present in the picture ( if any).	*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		txtdata		->	The destination text buffer.								*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	nf_debugprint((DEBUG_PREFIX "reader_get_txt\n"));
	(void)info;
	(void)txtdata;
}


/*==================================================================================*
 * boolean __CDECL reader_read:														*
 *		This function fits the buffer with image data								*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		buffer		->	The destination buffer.										*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	WebPInfo *myinfo = (WebPInfo *)info->_priv_ptr;
	int16_t i;

#define reader_cur_row(info) ((info)->_priv_var_more)

	if (reader_cur_row(info) < myinfo->canvas_height)
	{
		uint8_t *src = myinfo->bmap + reader_cur_row(info) * myinfo->stride_width;
		
		if (myinfo->channels == 4)
		{
			uint8_t red, green, blue;
			uint8_t r, g, b, a;
			uint8_t *dest = buffer;
			uint8_t *buf_ptr = src;
	
			for (i = info->width; i > 0; --i)
			{
				r = *buf_ptr++;
				g = *buf_ptr++;
				b = *buf_ptr++;
				a = *buf_ptr++;
	
				if (a == 255)
				{
					red = r;
					green = g;
					blue = b;
				} else if (a == 0)
				{
					red = 0xFF;
					green = 0xFF;
					blue = 0xFF;
				} else
				{
					uint32_t transparent_color = info->background_color;
	
					alpha_composite(red, r, a, (transparent_color >> 16) & 0xFF);
					alpha_composite(green, g, a, (transparent_color >> 8) & 0xFF);
					alpha_composite(blue, b, a, (transparent_color) & 0xFF);
				}

				*dest++ = red;
				*dest++ = green;
				*dest++ = blue;
			}
		} else
		{
			memcpy(buffer, src, myinfo->row_width);
		}
		reader_cur_row(info)++;
	}
	return TRUE;
}

/*==================================================================================*
 * boolean __CDECL reader_quit:														*
 *		This function makes the last job like close the file handler and free the	*
 *		allocated memory.															*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL reader_quit(IMGINFO info)
{
	WebPInfo *myinfo = (WebPInfo *)info->_priv_ptr;

	nf_debugprint((DEBUG_PREFIX "reader_quit\n"));
	if (myinfo)
	{
		WebPFree(myinfo->bmap);
		free(myinfo);
		info->_priv_ptr = NULL;
	}
}



/*==================================================================================*
 * boolean __CDECL encoder_init:													*
 *		Open the file "name", fit the "info" struct. ( see zview.h) and make others	*
 *		things needed by the encoder.												*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		name		->	The file to open.											*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL encoder_init(const char *name, IMGINFO info)
{
	(void)(name);

	info->planes   			= 24;
	info->colors  			= 16777215L;
	info->orientation		= UP_TO_DOWN;
	info->memory_alloc 		= TT_RAM;
	info->indexed_color	 	= FALSE;
	info->page			 	= 1;
	info->delay 		 	= 0;
	info->_priv_ptr	 		= 0;
	info->_priv_ptr_more	= NULL;
	info->__priv_ptr_more	= NULL;
	info->_priv_var	 		= 0;
	info->_priv_var_more	= 0;

	return TRUE;
}


/*==================================================================================*
 * boolean encoder_write:															*
 *		This function write data from buffer to file								*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		buffer		->	The source buffer.											*
 *		info		->	The IMGINFO struct		.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL encoder_write(IMGINFO info, uint8_t *buffer)
{
	(void) info;
	(void) buffer;
	return TRUE;
}


/*==================================================================================*
 * boolean __CDECL encoder_quit:													*
 *		This function makes the last job like close the file handler and free the	*
 *		allocated memory.															*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL encoder_quit(IMGINFO info)
{
	WebPInfo *myinfo = (WebPInfo *)info->_priv_ptr;

	if (myinfo)
	{
		if (myinfo->webp_file)
		{
			fclose(myinfo->webp_file);
			myinfo->webp_file = NULL;
		}
		free(myinfo);
		info->_priv_ptr = NULL;
	}
}