top_srcdir=../..
include $(top_srcdir)/configvars

PLUGINS = \
	aim \
	animastr \
	animatic \
	arabesqu \
	art \
	ascii \
	bmp \
	calamus \
	canvas \
	cel \
	coke \
	clrburst \
	colostar \
	colr \
	compeyes \
	crackart \
	cybrmate \
	dali \
	degas \
	degasx \
	delmpnt \
	deskpic \
	doodle \
	dungraph \
	duo \
	d_graph \
	eggpaint \
	esm \
	eureka \
	ezartpro \
	flic \
	fluff64 \
	fnt \
	ftc \
	fullshow \
	funnypnt \
	gfartani \
	gfartimg \
	gft \
	gif \
	godpaint \
	grafix \
	hrm \
	iff_dctv \
	iff_rgbn \
	imagelab \
	imagic \
	img \
	imgscan \
	indypnt \
	inshape \
	invision \
	jpg \
	macpaint \
	megapnt \
	mgf \
	mpp \
	mspaint \
	mtv \
	neoani \
	neochrom \
	pablopnt \
	paintpro \
	pblcpntr \
	pchrome \
	pci \
	pcl \
	pcx \
	picpackr \
	picworks \
	pixart \
	pl4 \
	png \
	pnm \
	pntshop \
	pntworks \
	pofo \
	prismpnt \
	pro89xx \
	psx \
	qoi \
	qpaint \
	qrt \
	rag_d \
	recoil \
	rembrndt \
	rgb \
	rle \
	seq \
	sgi \
	sgx \
	signum \
	spectrum \
	specx \
	spooky \
	ssb \
	stad \
	sun \
	svg \
	synarts \
	tga \
	tiff \
	tiny \
	uimg \
	zz_rough \
	webp \
	$(empty)

all:: $(PLUGINS)

.PHONY: $(PLUGINS)
$(PLUGINS): libshare
	$(MAKE) -C $@ all

libshare::
	$(MAKE) -C $@ all

dist:
	rm -rf ../_dist/codecs
	mkdir ../_dist/codecs
	rm -rf ../_dist/oldcodecs
	mkdir ../_dist/oldcodecs
	for f in $(PLUGINS); do \
		cp -a $$f/*.slb ../_dist/codecs; \
		cp -a $$f/*.ldg ../_dist/oldcodecs; \
	done
	if test -d purec/$(SLB_SUBDIR); then mkdir ../_dist/oldcodecs/purec; cp -a purec/$(SLB_SUBDIR)/* ../_dist/oldcodecs/purec; fi

clean:
	rm -f */*.ldg */*.slb */*.o */*.lo */*/*.o */*/*.lo */*/*/*.o */*/*/*.lo */*.a */*.pdb 
	$(MAKE) -C libshare clean
