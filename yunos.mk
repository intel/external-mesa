LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE:= mesa
LOCAL_ALIAS:= $(YUNOS_ROOT_ORIGIN)/third_party/mesa
LOCAL_MODULE_TAGS:= optional

ifeq ($(XMAKE_ARCH_TARGET),x86)
LOCAL_CMD_CONFIGURE:= \
       export CFLAGS="$${CPPFLAGS} $${CFLAGS} -I$${buildroot}/usr/include/libdrm/"; \
       export CXXFLAGS="$${CXXFLAGS} -I$${buildroot}/usr/include/libdrm/"; \
       export CPPFLAGS=""; \
       export LIBDRM_LIBS="-L$${buildroot}/usr/lib/ -ldrm"; \
       export EXPAT_LIBS="-L$${buildroot}/usr/lib/ -lexpat"; \
       export INTEL_LIBS="-L$${buildroot}/usr/lib/ -ldrm_intel -ldrm"; \
       export PTHREADSTUBS_LIBS="-L$${buildroot}/usr/lib/ -lpthread-stubs"; \
       export WAYLAND_SERVER_LIBS="-L$${buildroot}/usr/lib/ -lwayland-server"; \
       export WAYLAND_CLIENT_LIBS="-L$${buildroot}/usr/lib/ -lwayland-client"; \
       export ZLIB_LIBS="-L$${buildroot}/usr/lib/ -lz"; \
       ./autogen.sh  \
       --host=$(XMAKE_TOOLCHAIN_NAME) \
       --prefix=/usr \
       --disable-glx \
       --without-gallium-drivers \
       --enable-gbm \
       --enable-egl \
       --with-platforms=yunos,drm,wayland \
       --disable-dri3 \
       --with-dri-drivers=i965 \
       --enable-valgrind=no \
       --with-dri-searchpath=/usr/lib/dri:/usr/local/lib/dri

LOCAL_CMD_MAKE:= echo "start to build mesa, buildroot=$${buildroot}" ; \
       export CFLAGS="$${CPPFLAGS} $${CFLAGS} -I$${buildroot}/usr/include/libdrm/"; \
       export CXXFLAGS="$${CXXFLAGS} -I$${buildroot}/usr/include/libdrm/"; \
       export CPPFLAGS=""; \
       if [ "$${XMAKE_DISTCC}" = "true" ]; then \
            export CC="$${XMAKE_DISTCC_CC}";\
            export CXX="$${XMAKE_DISTCC_CXX}";\
       fi;\
       make CC="$${CC}" CXX="$${CXX}" V=1

LOCAL_CMD_MAKE_INSTALL:= echo "make install mesa"; \
        install -d $${buildroot}/usr/lib/egl; \
        make DESTDIR=$${buildroot} install; \
        mkdir -p $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib/egl/; \
        mkdir -p $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib/dri/; \
        cp -a $${buildroot}/usr/lib/egl/* $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib/egl/; \
        cp -a $${buildroot}/usr/lib/dri/*  $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib/dri/; \
        cp -a $${buildroot}/usr/lib/libglapi.*  $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib/; \
        cp -a $${buildroot}/usr/lib/libgbm.*  $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib/; \
        cp -a $${buildroot}/usr/lib/libwayland-egl.*  $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib/; \
        cp -a $${buildroot}/usr/lib/egl/* $${buildroot}/../obj/SHARED_LIBRARY/LINKED/; \
        cp -a $${buildroot}/usr/lib/dri/*  $${buildroot}/../obj/SHARED_LIBRARY/LINKED/; \
        cp -a $${buildroot}/usr/lib/libglapi.*  $${buildroot}/../obj/SHARED_LIBRARY/LINKED/; \
        cp -a $${buildroot}/usr/lib/libgbm.*  $${buildroot}/../obj/SHARED_LIBRARY/LINKED/; \
        cp -a $${buildroot}/usr/lib/libwayland-egl.*  $${buildroot}/../obj/SHARED_LIBRARY/LINKED/;
LOCAL_CMD_MAKE_CLEAN := rm -fr $(YUNOS_ROOT_ORIGIN)/$(XMAKE_ROOTFS)/usr/lib/egl
endif ### XMAKE_ARCH_TARGET = x86

ifeq ($(XMAKE_ARCH_TARGET),x86_64)
LOCAL_CMD_CONFIGURE:= \
       export CFLAGS="$${CPPFLAGS} $${CFLAGS} -I$${buildroot}/usr/include/libdrm/"; \
       export CXXFLAGS="$${CXXFLAGS} -I$${buildroot}/usr/include/libdrm/"; \
       export CPPFLAGS=""; \
       export LIBDRM_LIBS="-L$${buildroot}/usr/lib/ -ldrm"; \
       export EXPAT_LIBS="-L$${buildroot}/usr/lib/ -lexpat"; \
       export INTEL_LIBS="-L$${buildroot}/usr/lib/ -ldrm_intel -ldrm"; \
       export PTHREADSTUBS_LIBS="-L$${buildroot}/usr/lib/ -lpthread-stubs"; \
       export WAYLAND_SERVER_LIBS="-L$${buildroot}/usr/lib/ -lwayland-server"; \
       export WAYLAND_CLIENT_LIBS="-L$${buildroot}/usr/lib/ -lwayland-client"; \
       export ZLIB_LIBS="-L$${buildroot}/usr/lib/ -lz"; \
       ./autogen.sh  \
       --host=$(XMAKE_TOOLCHAIN_NAME) \
       --prefix=/usr \
       --disable-glx \
       --without-gallium-drivers \
       --enable-gbm \
       --enable-egl \
       --with-platforms=yunos,drm,wayland \
       --disable-dri3 \
       --with-dri-drivers=i965 \
       --enable-valgrind=no \
       --with-dri-searchpath=/usr/lib/dri:/usr/local/lib/dri

LOCAL_CMD_MAKE:= echo "start to build mesa, buildroot=$${buildroot}" ; \
       export CFLAGS="$${CFLAGS} -I$${buildroot}/usr/include/libdrm/"; \
       export CXXFLAGS="$${CXXFLAGS} -I$${buildroot}/usr/include/libdrm/"; \
       export CPPFLAGS="$${CPPFLAGS} -I$${buildroot}/usr/include/libdrm/"; \
       if [ "$${XMAKE_DISTCC}" = "true" ]; then \
            export CC="$${XMAKE_DISTCC_CC}";\
            export CXX="$${XMAKE_DISTCC_CXX}";\
       fi;\
       make CC="$${CC}" CXX="$${CXX}" V=1

LOCAL_CMD_MAKE_INSTALL:= echo "make install mesa"; \
        install -d $${buildroot}/usr/lib64/egl; \
        make install; \
        mkdir -p $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib64/egl/; \
        mkdir -p $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib64/dri/; \
        cp -a $${buildroot}/usr/lib64/egl/* $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib64/egl/; \
        cp -a $${buildroot}/usr/lib64/dri/*  $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib64/dri/; \
        cp -a $${buildroot}/usr/lib64/libglapi.*  $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib64/; \
        cp -a $${buildroot}/usr/lib64/libgbm.*  $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib64/; \
        cp -a $${buildroot}/usr/lib64/libwayland-egl.*  $${buildroot}/../obj/SYMBOLS/rootfs/usr/lib64/; \
        cp -a $${buildroot}/usr/lib64/egl/* $${buildroot}/../obj/SHARED_LIBRARY/LINKED/; \
        cp -a $${buildroot}/usr/lib64/dri/*  $${buildroot}/../obj/SHARED_LIBRARY/LINKED/; \
        cp -a $${buildroot}/usr/lib64/libglapi.*  $${buildroot}/../obj/SHARED_LIBRARY/LINKED/; \
        cp -a $${buildroot}/usr/lib64/libgbm.*  $${buildroot}/../obj/SHARED_LIBRARY/LINKED/; \
        cp -a $${buildroot}/usr/lib64/libwayland-egl.*  $${buildroot}/../obj/SHARED_LIBRARY/LINKED/;
LOCAL_CMD_MAKE_CLEAN := rm -fr $(YUNOS_ROOT_ORIGIN)/$(XMAKE_ROOTFS)/usr/lib64/egl
endif ### XMAKE_ARCH_TARGET = x86_64

#LOCAL_CMD_MAKE_CLEAN:= echo "ignore"
LOCAL_CMD_MAKE_DISTCLEAN:= echo "ignore"
LOCAL_REQUIRED_MODULES:= python \
                                  python-lxml

LOCAL_SHARED_LIBRARIES:= libpciaccess libdrm_intel libxml2 libexpat libpthread-stubs libgfx-cutils libhal

LOCAL_LDLIBS += -lwayland-client -lwayland-server
ifeq ($(XMAKE_ENABLE_WAYLAND_3), true)
LOCAL_SHARED_LIBRARIES += libwayland-client libwayland-server
else
LOCAL_REQUIRED_MODULES += wayland
endif



#LOCAL_REQUIRED_MODULES:= python systemd wayland libpciaccess libdrm \
                                  libxml2 \
                                  expat python-lxml libpthread-stubs libgfx-cutils
LOCAL_CMD_PRE:= echo "mesa: fake make tag: pre"
LOCAL_CMD_POST:= echo "mesa: fake make tag: post"
LOCAL_CMD_PRE_PREBUILT:= echo "mesa: fake make tag: pre-prebuilt"
LOCAL_CMD_POST_PREBUILT:= echo "mesa: fake make tag: post-prebuilt"
LOCAL_CMD_PRE_IMAGE:= echo "mesa: fake make tag: pre-image"
LOCAL_CMD_IMAGE:= echo "mesa: fake make tag: image"
LOCAL_CMD_POST_IMAGE:= echo "mesa: fake make tag: post-image"
LOCAL_CMD_PRE_IMAGE_STRIPPED:= echo "mesa: fake make tag: pre-image-stripped"
LOCAL_CMD_IMAGE_STRIPPED:= echo "mesa: fake make tag: image-stripped"
LOCAL_CMD_POST_IMAGE_STRIPPED:= echo "mesa: fake make tag: post-image-stripped"
include $(BUILD_MODULE)
