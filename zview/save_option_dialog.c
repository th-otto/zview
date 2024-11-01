#include "general.h"
#include "prefs.h"
#include "ztext.h"
#include "pic_load.h"
#include "catalog/catalog.h"
#include "menu.h"
#include "plugins.h"
#include "plugins/common/zvplugin.h"
#include "close_modal.h"

#define	    COMPRESSION_NONE		1		/* dump mode */
#define	    COMPRESSION_PACKBITS	32773	/* Macintosh RLE */
#define	    COMPRESSION_LZW			5       /* Lempel-Ziv  & Welch */
#define	    COMPRESSION_JPEG		7		/* JPEG DCT compression */
#define	    COMPRESSION_DEFLATE		32946	/* Deflate compression */

static void *option_slider;

static int16 quality = 90;
static J_COLOR_SPACE color_space = JCS_RGB;
static boolean progressive = FALSE;
#if 0
static boolean show_preview = FALSE;
#endif
static uint16 compression = COMPRESSION_NONE;
static int compression_button = 0;

static const char *const colorspace_items[] = {
	"Greyscale",
	"RGB",
	"Y/Cb/Cr",
	"CMYK",
	"YCCK"
};

static void __CDECL save_option_close(WINDOW *win EVNT_BUFF_PARAM)
{
	free(option_slider);
	option_slider = NULL;
	frm_cls(win);
}

static void colorspace_popup(WINDOW *win, int obj_index)
{
	int choice;
	int16_t x, y;
	
	objc_offset(FORM(win), obj_index, &x, &y);
	choice = MenuPopUp((void *)colorspace_items, x, y, (int)(sizeof(colorspace_items) / sizeof(colorspace_items[0])), -1, color_space, P_LIST | P_WNDW | P_CHCK);
	if (choice > 0)
	{
		if (color_space != choice)
		{
			strcpy(FORM(win)[PREF_COLORSPACE].ob_spec.free_string, colorspace_items[choice - 1]);
			ObjcDraw(OC_FORM, win, PREF_COLORSPACE, 1);
		}
		color_space = choice;
	}
}


static void option_progressive_event(WINDOW *win, int obj_index, int mode, void *data)
{
	progressive = !progressive;

	/* a wait loop while the mouse button is pressed */		
	while (evnt.mbut == 1 || evnt.mbut == 2)
		graf_mkstate(&evnt.mx, &evnt.my, &evnt.mbut, &evnt.mkstate); 
}

#if 0
static void option_show_preview(WINDOW *win, int obj_index, int mode, void *data)
{
	show_preview = !show_preview;

	/* a wait loop while the mouse button is pressed */		
	while (evnt.mbut == 1 || evnt.mbut == 2)
		graf_mkstate(&evnt.mx, &evnt.my, &evnt.mbut, &evnt.mkstate); 
}
#endif


static void __CDECL option_dialog_event( WINDOW *win EVNT_BUFF_PARAM)
{
	int16 object = EVNT_BUFF[4];

	switch (object)
	{
	case PREF_COLORSPACE:
		colorspace_popup(win, object);
		ObjcChange(OC_FORM, win, object, NORMAL, TRUE);
		break;
	}	
}


static void __CDECL option_ok_event(WINDOW *win, int obj_index, int mode, void *data)
{
	switch (compression_button)
	{ 
	case TIFFPREF_DEFLATE:
		compression = COMPRESSION_DEFLATE;
		break;	
	case TIFFPREF_JPEG:
		compression = COMPRESSION_JPEG;
		break;	
	case TIFFPREF_LZW:
		compression = COMPRESSION_LZW;
		break;	
	case TIFFPREF_RLE:
		compression = COMPRESSION_PACKBITS;
		break;	
	default:
		compression = COMPRESSION_NONE;
		break;		
	}

	if (curr_output_plugin)
	{
		switch (curr_output_plugin->type)
		{
		case CODEC_SLB:
			plugin_set_option(&curr_output_plugin->c.slb, OPTION_QUALITY, quality);
			plugin_set_option(&curr_output_plugin->c.slb, OPTION_COMPRESSION, compression);
			plugin_set_option(&curr_output_plugin->c.slb, OPTION_COLOR_SPACE, color_space);
			plugin_set_option(&curr_output_plugin->c.slb, OPTION_PROGRESSIVE, progressive);
			break;
		case CODEC_LDG:
			{
				struct _ldg_funcs *funcs;

				funcs = curr_output_plugin->c.ldg.funcs;
				if (funcs->set_tiff_option)
				{
					funcs->set_tiff_option(quality, compression);
				} else if (funcs->set_jpg_option)
				{
					funcs->set_jpg_option(quality, color_space, progressive);
				}
			}
			break;
		}
	}

	ObjcChange(OC_FORM, win, obj_index, NORMAL, TRUE);
	close_win(win);
}


static void __CDECL option_slider_event(WINDOW *win, int mode,
#if __WINDOM_MAJOR__ >= 2
	short value,
#else
	float value,
#endif
	void *data)
{
	(void)data;
	quality = (int16)value;
	evnt_timer(50);
	sprintf(ObjcString(FORM(win), PREF_PERCENT, NULL), "%d", quality);
	ObjcDraw(mode, win, PREF_PERCENT, TRUE);
}

/*==================================================================================*
 * void save_option_dialog:															*
 *		Open and handle encoder option dialog.										*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		source_file:		the full path name of the source file.					*
 *----------------------------------------------------------------------------------*
 * returns: 																		*
 *      --																			*
 *==================================================================================*/
void save_option_dialog(const char *source_file, CODEC *codec)
{
	WINDOW 	*win;
	OBJECT *option_content;
	unsigned int option_mask;

	option_content = get_tree(SAVEPREF);
	option_mask = get_option_mask(codec);

	if (curr_output_plugin)
	{
		switch (curr_output_plugin->type)
		{
		case CODEC_SLB:
			{
				long value;

				if ((value = plugin_get_option(&curr_output_plugin->c.slb, OPTION_QUALITY)) >= 0)
					quality = value;
				if ((value = plugin_get_option(&curr_output_plugin->c.slb, OPTION_COLOR_SPACE)) >= 0)
					color_space = value;
				if ((value = plugin_get_option(&curr_output_plugin->c.slb, OPTION_PROGRESSIVE)) >= 0)
					progressive = value;
				if ((value = plugin_get_option(&curr_output_plugin->c.slb, OPTION_COMPRESSION)) >= 0)
					compression = value;
			}
			break;
		}
	}

	sprintf(ObjcString(option_content, PREF_PERCENT, NULL), "%d", quality);

	option_content[PREF_SL].ob_x = quality;

	strcpy(option_content[PREF_COLORSPACE].ob_spec.free_string, colorspace_items[color_space - 1]);

	if ((win = FormCreate(option_content, NAME|MOVER, option_dialog_event, get_string(SAVE_OPTION_TITLE), NULL, TRUE, FALSE)) == NULL)
	{
		errshow(NULL, ALERT_WINDOW);	
		return;
	}

#if 0
	RsrcUserDraw(OC_FORM, win, PREF_ORIGINAL, draw_object_image, NULL);
	RsrcUserDraw(OC_FORM, win, PREF_RESULT, draw_object_image, NULL);
#endif

	WindSet(win, WF_BEVENT, BEVENT_MODAL, 0, 0, 0);

	EvntAttach(win, WM_CLOSED, save_option_close);

	if (option_mask & (1 << OPTION_QUALITY))
	{
		if ((option_slider = SlidCreate(1, 100, 1, 10, quality, SLD_HORI, SLD_IMME)) == NULL)
		{
			frm_cls(win);
			errshow(NULL, -ENOMEM);
			return;	
		}	
	
		SlidAttach(option_slider, OC_FORM, win, PREF_LF, PREF_PG, PREF_SL, PREF_RT);
		SlidSetFunc(option_slider, option_slider_event, NULL);
	}

#if 0
	ObjcAttachFormFunc(win, PREF_PREVIEW, option_show_preview, NULL);
#endif

	if (option_mask & (1 << OPTION_COLOR_SPACE))
	{
		ObjcChange(OC_FORM, win, PREF_COLORSPACE, ~OS_DISABLED, TRUE);
	} else
	{
		ObjcChange(OC_FORM, win, PREF_COLORSPACE, OS_DISABLED, TRUE);
	}

	if (option_mask & (1 << OPTION_PROGRESSIVE))
	{
		ObjcAttachFormFunc(win, PREF_PROGRESSIVE, option_progressive_event, NULL);
		ObjcChange(OC_FORM, win, PREF_PROGRESSIVE, ~OS_DISABLED, TRUE);
	} else
	{
		ObjcChange(OC_FORM, win, PREF_PROGRESSIVE, OS_DISABLED, TRUE);
	}

	if (option_mask & (1 << OPTION_COMPRESSION))
	{
		if (compression == COMPRESSION_NONE)
			ObjcChange(OC_FORM, win, TIFFPREF_NONE, OS_SELECTED, TRUE);	
		
		ObjcAttachVar(OC_FORM, win, TIFFPREF_NONE, &compression_button, TIFFPREF_NONE);
		ObjcAttachVar(OC_FORM, win, TIFFPREF_RLE, &compression_button, TIFFPREF_RLE);
		ObjcAttachVar(OC_FORM, win, TIFFPREF_LZW, &compression_button, TIFFPREF_LZW);
		ObjcAttachVar(OC_FORM, win, TIFFPREF_JPEG, &compression_button, TIFFPREF_JPEG);
		ObjcAttachVar(OC_FORM, win, TIFFPREF_DEFLATE, &compression_button, TIFFPREF_DEFLATE);
		ObjcChange(OC_FORM, win, TIFFPREF_NONE, ~OS_DISABLED, TRUE);
		ObjcChange(OC_FORM, win, TIFFPREF_RLE, ~OS_DISABLED, TRUE);
		ObjcChange(OC_FORM, win, TIFFPREF_LZW, ~OS_DISABLED, TRUE);
		ObjcChange(OC_FORM, win, TIFFPREF_JPEG, ~OS_DISABLED, TRUE);
		ObjcChange(OC_FORM, win, TIFFPREF_DEFLATE, ~OS_DISABLED, TRUE);
	} else
	{
		ObjcChange(OC_FORM, win, TIFFPREF_NONE, OS_DISABLED, TRUE);
		ObjcChange(OC_FORM, win, TIFFPREF_RLE, OS_DISABLED, TRUE);
		ObjcChange(OC_FORM, win, TIFFPREF_LZW, OS_DISABLED, TRUE);
		ObjcChange(OC_FORM, win, TIFFPREF_JPEG, OS_DISABLED, TRUE);
		ObjcChange(OC_FORM, win, TIFFPREF_DEFLATE, OS_DISABLED, TRUE);
	}

#if 0
	ObjcAttachFormFunc(win, PREF_PREVIEW, option_show_preview, NULL);
#endif

	ObjcAttachFormFunc(win, PREF_OK, option_ok_event, NULL);
}
