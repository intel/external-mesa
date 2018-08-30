/*
 *
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2016  Intel Corporation
 *
 * Based on platform_x11.c and platform_android.c, which have :
 *
 * Copyright (C) 2011 Intel Corporation
 * Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
 * Copyright (C) 2010-2011 LunarG Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Long, Zhifang <zhifang.long@intel.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <xf86drm.h>
#include <libsync.h>

#include <cutils/graphics.h>
#include <cutils/native_surface.h>
#include <cutils/yalloc.h>
#include <log/Log.h>

#include "yalloc_drm.h"

#include "loader.h"
#include "egl_dri2.h"
#include "egl_dri2_fallbacks.h"

#define LOG_TAG "MESA"


#define ALIGN(val, align)	(((val) + (align) - 1) & ~((align) - 1))


static void yunos_log(EGLint level, const char *msg)
{
   switch (level) {
   case _EGL_DEBUG:
      LOG_I("%s", msg);
      break;
   case _EGL_INFO:
      LOG_I("%s", msg);
      break;
   case _EGL_WARNING:
      LOG_W("%s", msg);
      break;
   case _EGL_FATAL:
      LOG_E("%s", msg);
      break;
   default:
      break;
   }

}


static int
yunos_open_device(struct dri2_egl_display *dri2_dpy)
{
   int fd = 0;
   int err = 0;
   int buf_type = YALLOC_BUFFER_TYPE_GEM_NAME;
   struct yalloc_device_t *yalloc = NULL;

   VendorModule* m = (VendorModule*) LOAD_VENDOR_MODULE(YALLOC_VENDOR_MODULE_ID);
   if (m == NULL) {
       _eglLog(_EGL_FATAL, "%s:%d\n", __func__, __LINE__);
       return -1;
   }
   err = yalloc_open(m, &yalloc);
   if(!err) {
      dri2_dpy->yalloc = yalloc;

      err = yalloc->dispose(yalloc, YALLOC_DISPOSE_GET_DRM_FD, &fd);
      if(!err && (fd >= 0)) {
         dri2_dpy->fd = fd;//fcntl(fd, F_DUPFD_CLOEXEC, 3);

         yalloc->dispose(yalloc, YALLOC_DISPOSE_GET_BUFFER_TYPE, &buf_type);
         dri2_dpy->is_buf_prime_fd = (buf_type ==  YALLOC_BUFFER_TYPE_PRIME_FD) ? 1 : 0;
      }
   }

	_eglLog(_EGL_DEBUG, "%s:%d : yalloc = 0x%08x, fd = %d, is_prime_fd = %d, err =%d \n",
      __func__, __LINE__, (unsigned int)(dri2_dpy->yalloc), dri2_dpy->fd, dri2_dpy->is_buf_prime_fd, err);
	return  (!err);
}


static int
yunos_get_native_buffer_bpp(struct yalloc_device_t *yalloc, struct NativeSurfaceBuffer *buf)
{
   int bpp = 0;
   if(yalloc && buf) {
         yalloc->dispose(yalloc, YALLOC_DISPOSE_GET_PLANE_BPP, buf->target, &bpp);
   }

   if(bpp == 0) {
      _eglLog(_EGL_WARNING, "%s:%d : bpp returns %d", __func__, __LINE__, bpp);
   }
   return bpp;
}

static int
yunos_get_native_buffer_pitch(struct yalloc_device_t *yalloc, struct NativeSurfaceBuffer *buf)
{
   int pitch = 0;
   int bpp = 0;
   int x_stride = 0;
   if(yalloc && buf) {
         yalloc->dispose(yalloc, YALLOC_DISPOSE_GET_PLANE_BPP, buf->target, &bpp);
         yalloc->dispose(yalloc, YALLOC_DISPOSE_GET_X_STRIDE, buf->target, &x_stride);
         pitch = bpp * x_stride;
   }

   if(pitch == 0) {
      _eglLog(_EGL_WARNING, "%s:%d : pitch returns %d", __func__, __LINE__, pitch);
   }
   return pitch;
}

static int
yunos_get_native_buffer_plane_num(struct yalloc_device_t *yalloc, struct NativeSurfaceBuffer *buf)
{
   int plane_num = 0;
   if(yalloc && buf) {
         yalloc->dispose(yalloc, YALLOC_DISPOSE_GET_PLANE_NUM, buf->target, &plane_num);
   }

   if(plane_num == 0) {
      _eglLog(_EGL_WARNING, "%s:%d : bpp returns %d", __func__, __LINE__, plane_num);
   }
   return plane_num;
}

static void
yunos_get_native_buffer_plane_offsets(struct yalloc_device_t *yalloc, struct NativeSurfaceBuffer *buf, int* offsets)
{
   if(yalloc && buf) {
      yalloc->dispose(yalloc, YALLOC_DISPOSE_GET_PLANE_OFFSETS, buf->target, offsets);
   }
}

static void
yunos_get_native_buffer_plane_pitches(struct yalloc_device_t *yalloc, struct NativeSurfaceBuffer *buf, int* pitches)
{
   int plane_num = 0;
   int bpps[YALLOC_MAX_PLANE_NUM];
   int x_strides[YALLOC_MAX_PLANE_NUM];
   int i = 0;

   if(yalloc && buf) {
      yalloc->dispose(yalloc, YALLOC_DISPOSE_GET_PLANE_NUM, buf->target, &plane_num);
      yalloc->dispose(yalloc, YALLOC_DISPOSE_GET_PLANE_BPPS, buf->target, bpps);
      yalloc->dispose(yalloc, YALLOC_DISPOSE_GET_X_STRIDES, buf->target, x_strides);

      for (i = 0; i < plane_num; i++) {
         pitches[i] = bpps[i] * x_strides[i];
      }
   }
}

static int
yunos_get_native_buffer_fd(struct yalloc_device_t *yalloc, struct NativeSurfaceBuffer *buf)
{
   int fd = -1;
   if(yalloc && buf) {
      yalloc->dispose(yalloc, YALLOC_DISPOSE_GET_BUFFER_FD, buf->target, &fd);
   }
   return fd;
}

static int yunos_get_native_buffer_name(struct yalloc_device_t *yalloc, struct NativeSurfaceBuffer *buf)
{
   int name = 0;
   if(yalloc && buf) {
      yalloc->dispose(yalloc, YALLOC_DISPOSE_GET_BUFFER_NAME, buf->target, &name);
   }

   return name;
}



/* TODO :
    Revise __DRI_IMAGE_FOURCC_xxx to cover all hal formats, or directly get fourcc from yalloc
*/
static int
yunos_get_fourcc(int native_format)
{
	int ret = 0;

	switch (native_format) {
   case YUN_HAL_FORMAT_RGBA_8888:
   case YUN_HAL_FORMAT_sRGB_A_8888:
      ret = __DRI_IMAGE_FOURCC_ABGR8888;
      break;

	case YUN_HAL_FORMAT_RGBX_8888:
   case YUN_HAL_FORMAT_sRGB_X_8888:
      ret = __DRI_IMAGE_FOURCC_XBGR8888;
      break;

   case YUN_HAL_FORMAT_RGB_565:
	   ret = __DRI_IMAGE_FOURCC_RGB565;
		break;

   case YUN_HAL_FORMAT_BGRA_8888:
   case YUN_HAL_FORMAT_sBGR_A_8888:
      ret = __DRI_IMAGE_FOURCC_ARGB8888;
		break;

   case YUN_HAL_FORMAT_BGRX_8888:
   case YUN_HAL_FORMAT_sBGR_X_8888:
      ret = __DRI_IMAGE_FOURCC_XRGB8888;
      break;

   case YUN_HAL_FORMAT_I420:
      ret = __DRI_IMAGE_FOURCC_YUV420;
      break;

	case YUN_HAL_FORMAT_YV12:
		ret = __DRI_IMAGE_FOURCC_YVU420;
		break;

   case YUN_HAL_FORMAT_NV12:
   case YUN_HAL_FORMAT_DRM_NV12:
      ret = __DRI_IMAGE_FOURCC_NV12;
      break;

   case YUN_HAL_FORMAT_NV21:
   case YUN_HAL_FORMAT_YCrCb_420_SP:
      ret = __DRI_IMAGE_FOURCC_NV21;
      break;

   case YUN_HAL_FORMAT_NV16:
   case YUN_HAL_FORMAT_YCbCr_422_SP:
      ret = __DRI_IMAGE_FOURCC_NV16;
      break;

   case YUN_HAL_FORMAT_YUYV:
   case YUN_HAL_FORMAT_YCbCr_422_I:
      ret = __DRI_IMAGE_FOURCC_YUYV;
      break;

   case YUN_HAL_FORMAT_RGB_888:
   case YUN_HAL_FORMAT_Y8:
   case YUN_HAL_FORMAT_Y16:
   case YUN_HAL_FORMAT_NV61:
   case YUN_HAL_FORMAT_YCbCr_420_888:
	default:
			ret = -1;
		   _eglLog(_EGL_WARNING, "%s:%d : unsupported format 0x%08x", __func__, __LINE__, native_format);
         break;
	}

   return ret;
}

/* TODO :
    Revise __DRI_IMAGE_FORMAT_xxx to cover all hal formats, or directly get fourcc from yalloc
*/
static int
yunos_get_format(int native_format)
{
	int ret = 0;

	switch (native_format) {
   case YUN_HAL_FORMAT_RGBA_8888:
   case YUN_HAL_FORMAT_sRGB_A_8888:
      ret = __DRI_IMAGE_FORMAT_ABGR8888;
      break;

	case YUN_HAL_FORMAT_RGBX_8888:
   case YUN_HAL_FORMAT_sRGB_X_8888:
      ret = __DRI_IMAGE_FORMAT_XBGR8888;
      break;

   case YUN_HAL_FORMAT_RGB_565:
	   ret = __DRI_IMAGE_FORMAT_RGB565;
		break;

   case YUN_HAL_FORMAT_BGRA_8888:
   case YUN_HAL_FORMAT_sBGR_A_8888:
      ret = __DRI_IMAGE_FORMAT_ARGB8888;
		break;

   case YUN_HAL_FORMAT_BGRX_8888:
   case YUN_HAL_FORMAT_sBGR_X_8888:
      ret = __DRI_IMAGE_FORMAT_XRGB8888;
      break;

   case YUN_HAL_FORMAT_I420:
      ret = __DRI_IMAGE_FOURCC_YUV420;
      break;

	case YUN_HAL_FORMAT_YV12:
		ret = __DRI_IMAGE_FOURCC_YVU420;
		break;

   case YUN_HAL_FORMAT_NV12:
   case YUN_HAL_FORMAT_DRM_NV12:
      ret = __DRI_IMAGE_FOURCC_NV12;
      break;

   case YUN_HAL_FORMAT_NV21:
   case YUN_HAL_FORMAT_YCrCb_420_SP:
      ret = __DRI_IMAGE_FOURCC_NV21;
      break;

   case YUN_HAL_FORMAT_NV16:
   case YUN_HAL_FORMAT_YCbCr_422_SP:
      ret = __DRI_IMAGE_FOURCC_NV16;
      break;

   case YUN_HAL_FORMAT_YUYV:
   case YUN_HAL_FORMAT_YCbCr_422_I:
      ret = __DRI_IMAGE_FOURCC_YUYV;
      break;

   case YUN_HAL_FORMAT_RGB_888:
   case YUN_HAL_FORMAT_Y8:
   case YUN_HAL_FORMAT_Y16:
   case YUN_HAL_FORMAT_NV61:
   case YUN_HAL_FORMAT_UYVY:
   case YUN_HAL_FORMAT_VYUY:
   case YUN_HAL_FORMAT_YVYU:
   case YUN_HAL_FORMAT_YCbCr_420_888:
      break;

	default:
			ret = -1;
		   _eglLog(_EGL_WARNING, "%s:%d : unsupported format 0x%08x", __func__, __LINE__, native_format);
	}

   return ret;
}


static EGLBoolean
yunos_surface_obtain_buffer(struct dri2_egl_surface *dri2_surf)
{
	int fence_fd;

	_eglLog(_EGL_DEBUG, "%s:%d", __func__, __LINE__);

	if (dri2_surf->surface->obtainBuffer(dri2_surf->surface, &dri2_surf->buffer, &fence_fd)) {
    _eglLog(_EGL_DEBUG, "%s:%d obtainBuffer failed", __func__, __LINE__);
    return EGL_FALSE;
  }

  if (fence_fd >= 0) {
    /* From the SYNC_IOC_WAIT documentation in <linux/sync.h>:
     *
     *    Waits indefinitely if timeout < 0.
     */
    int timeout = -1;
    sync_wait(fence_fd, timeout);
    close(fence_fd);
  }

  dri2_surf->buffer->base.incRef(&dri2_surf->buffer->base);

	return EGL_TRUE;
}


static EGLBoolean
yunos_surface_submit_buffer(_EGLDisplay *disp, struct dri2_egl_surface *dri2_surf)
{
	/* FIXME : whehter we need unlock/lock disp->Mutex here ? */
  _eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   _EGLContext *ctx = _eglGetCurrentContext();
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);

   int fence_fd;

   if(dri2_ctx) {
      void * fence = NULL;
      fence = dri2_dpy->fence->create_fence(dri2_ctx->dri_context);
      if (fence) {
         dri2_dpy->fence->client_wait_sync(dri2_ctx->dri_context, fence,
            __DRI2_FENCE_FLAG_FLUSH_COMMANDS, 50000000);
         dri2_dpy->fence->destroy_fence(dri2_dpy->dri_screen, fence);
         fence = NULL;
      }
   }

   if (dri2_surf->dri_image) {
      dri2_dpy->image->destroyImage(dri2_surf->dri_image);
      dri2_surf->dri_image = NULL;
   }

   fence_fd = dri2_surf->out_fence_fd;
   dri2_surf->out_fence_fd = -1;
   dri2_surf->surface->submitBuffer(dri2_surf->surface, dri2_surf->buffer, fence_fd);
   dri2_surf->buffer->base.decRef(&dri2_surf->buffer->base);
   dri2_surf->buffer = NULL;

   return EGL_TRUE;
}

static void
yunos_surface_drop_buffer(_EGLDisplay *disp, struct dri2_egl_surface *dri2_surf)
{
  _eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);
  int ret;
  int fence_fd = dri2_surf->out_fence_fd;
  dri2_surf->out_fence_fd = -1;
  ret = dri2_surf->surface->dropBuffer(dri2_surf->surface, dri2_surf->buffer, fence_fd);
  if (ret < 0) {
    _eglLog(_EGL_WARNING, "yunos_surface_drop_buffer dropBuffer failed");
    dri2_surf->base.Lost = EGL_TRUE;
  }
}

static __DRIbuffer *
yunos_alloc_local_buffer(struct dri2_egl_surface *dri2_surf,
                         unsigned int att, unsigned int format)
{_eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   if (att >= ARRAY_SIZE(dri2_surf->local_buffers))
      return NULL;

   if (!dri2_surf->local_buffers[att]) {
      dri2_surf->local_buffers[att] =
         dri2_dpy->dri2->allocateBuffer(dri2_dpy->dri_screen, att, format,
               dri2_surf->base.Width, dri2_surf->base.Height);
   }

   return dri2_surf->local_buffers[att];
}

static void
yunos_free_local_buffers(struct dri2_egl_surface *dri2_surf)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   int i;

   for (i = 0; i < ARRAY_SIZE(dri2_surf->local_buffers); i++) {
      if (dri2_surf->local_buffers[i]) {
         dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
               dri2_surf->local_buffers[i]);
         dri2_surf->local_buffers[i] = NULL;
      }
   }
}

static _EGLSurface *
yunos_create_surface(_EGLDriver *drv, _EGLDisplay *disp, EGLint type,
		    _EGLConfig *conf, void *native_window,
		    const EGLint *attrib_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct dri2_egl_surface *dri2_surf;
   struct NativeSurface *surface = native_window;
   const __DRIconfig *config;
   __DRIcreateNewDrawableFunc createNewDrawable;

   _eglLog(_EGL_DEBUG, "%s : %d : ", __func__, __LINE__);

   dri2_surf = calloc(1, sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "yunos_create_surface");
      return NULL;
   }

   if (!dri2_init_surface(&dri2_surf->base, disp, type, conf, attrib_list, true))
      goto cleanup_surface;

   if (type == EGL_WINDOW_BIT) {
      int format;

      if (!surface) {
         _eglError(EGL_BAD_NATIVE_WINDOW, "yunos_create_surface");
         goto cleanup_surface;
      }
      if (surface->lookup(surface, NATIVE_SURFACE_FORMAT, &format)) {
         _eglError(EGL_BAD_NATIVE_WINDOW, "yunos_create_surface");
         goto cleanup_surface;
      }

      if (format != dri2_conf->base.NativeVisualID) {
         _eglLog(_EGL_WARNING, "yunos_create_surface : native format mismatch: 0x%x != 0x%x",
               format, dri2_conf->base.NativeVisualID);
      }

      surface->lookup(surface, NATIVE_SURFACE_WIDTH, &dri2_surf->base.Width);
      surface->lookup(surface, NATIVE_SURFACE_HEIGHT, &dri2_surf->base.Height);
   }

   config = dri2_get_dri_config(dri2_conf, type,
                                dri2_surf->base.GLColorspace);
   if (!config) {
      _eglError(EGL_BAD_CONFIG, "dri2_get_dri_config return NULL");
      goto cleanup_surface;
   }

   if (dri2_dpy->image_driver)
      createNewDrawable = dri2_dpy->image_driver->createNewDrawable;
   else
      createNewDrawable = dri2_dpy->dri2->createNewDrawable;

   dri2_surf->dri_drawable =
      (*createNewDrawable)(dri2_dpy->dri_screen, config, dri2_surf);


   if (dri2_surf->dri_drawable == NULL) {
      _eglError(EGL_BAD_ALLOC, "dri2->createNewDrawable");
      goto cleanup_surface;
   }


   if (surface) {
      surface->base.incRef(&surface->base);
      dri2_surf->surface = surface;
   }

   return &dri2_surf->base;

cleanup_surface:
   free(dri2_surf);

   return NULL;
}

static int
yunos_update_buffers(struct dri2_egl_surface *dri2_surf)
{
  if (dri2_surf->base.Lost)
    return -1;

  if (dri2_surf->base.Type != EGL_WINDOW_BIT)
    return 0;

  /* try to get the next back buffer */
  if (!dri2_surf->buffer && !yunos_surface_obtain_buffer(dri2_surf)) {
    _eglLog(_EGL_WARNING, "Could not dequeue buffer from native window");
    dri2_surf->base.Lost = EGL_TRUE;
    return -1;
  }

  /* free outdated buffers and update the surface size */
  if (dri2_surf->base.Width != dri2_surf->buffer->width ||
       dri2_surf->base.Height != dri2_surf->buffer->height) {
    dri2_egl_surface_free_local_buffers(dri2_surf);
    dri2_surf->base.Width = dri2_surf->buffer->width;
    dri2_surf->base.Height = dri2_surf->buffer->height;
  }

  return 0;
}

static int
yunos_get_front_bo(struct dri2_egl_surface *dri2_surf, unsigned int format)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
      _eglLog(_EGL_WARNING, "Front buffer is not supported for window surfaces");
      return -1;
   }

   if (dri2_surf->dri_image2)
      return 0;

   dri2_surf->dri_image2 =
      dri2_dpy->image->createImage(dri2_dpy->dri_screen,
                                   dri2_surf->base.Width,
                                   dri2_surf->base.Height,
                                   format,
                                   0,
                                   dri2_surf);

   if (!dri2_surf->dri_image2)
   {
      _eglLog(_EGL_WARNING, "%s:%d dri2_image2 allocation failed !\n", __func__, __LINE__);
      return -1;
   }

   return 0;

}


static int
yunos_get_back_bo(struct dri2_egl_surface *dri2_surf, unsigned int format)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   int fourcc, pitch;
   int offset = 0;
   int fd = -1;
   int name = 0;

   if (dri2_surf->base.Type != EGL_WINDOW_BIT) {
      _eglLog(_EGL_WARNING, "Back buffer is not supported for pbuffer surfaces");
      return -1;
   }

   if (dri2_surf->dri_image)
      return 0;

   if (!dri2_surf->buffer)
      return -1;

   fd = yunos_get_native_buffer_fd(dri2_dpy->yalloc, dri2_surf->buffer);
   if (fd < 0) {
      name = yunos_get_native_buffer_name(dri2_dpy->yalloc, dri2_surf->buffer);
      if (!name) {
         _eglLog(_EGL_WARNING, "%s:%d can't get fd or name from buffer !\n", __func__, __LINE__);
         return -1;
      }
   }

   if(fd < 0) {
       dri2_surf->dri_image =
           dri2_dpy->image->createImageFromName(dri2_dpy->dri_screen,
                                               dri2_surf->buffer->width,
                                               dri2_surf->buffer->height,
                                               format,
                                               name,
                                               dri2_surf->buffer->stride,
                                               dri2_surf);
   }
   else {
      fourcc = yunos_get_fourcc(dri2_surf->buffer->format);
      pitch = yunos_get_native_buffer_pitch(dri2_dpy->yalloc, dri2_surf->buffer);
      if (fourcc == -1 || pitch == 0) {
         return -1;
      }

      dri2_surf->dri_image =
         dri2_dpy->image->createImageFromFds(dri2_dpy->dri_screen,
                                             dri2_surf->base.Width,
                                             dri2_surf->base.Height,
                                             fourcc,
                                             &fd,
                                             1,
                                             &pitch,
                                             &offset,
                                             dri2_surf);
   }

   if (!dri2_surf->dri_image) {
      _eglLog(_EGL_WARNING, "%s:%d dri_image allocation failed !\n", __func__, __LINE__);
      return -1;
   }

   return 0;
}


static int
yunos_get_buffers_parse_attachments(struct dri2_egl_surface *dri2_surf,
                                    unsigned int *attachments, int count)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   int num_buffers = 0, i;
   _eglLog(_EGL_DEBUG, "%s:%d :\n", __func__, __LINE__);

   /* fill dri2_surf->buffers */
   for (i = 0; i < count * 2; i += 2) {
      __DRIbuffer *buf, *local;

      assert(num_buffers < ARRAY_SIZE(dri2_surf->buffers));
      buf = &dri2_surf->buffers[num_buffers];

      switch (attachments[i]) {
      case __DRI_BUFFER_BACK_LEFT:
         if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
            buf->attachment = attachments[i];
            buf->name = yunos_get_native_buffer_name(dri2_dpy->yalloc, dri2_surf->buffer);
            buf->cpp = yunos_get_native_buffer_bpp(dri2_dpy->yalloc, dri2_surf->buffer);
            buf->pitch = yunos_get_native_buffer_pitch(dri2_dpy->yalloc, dri2_surf->buffer);
            buf->flags = 0;

            if (buf->name)
               num_buffers++;

            break;
         }

         /* fall through for pbuffers */
      case __DRI_BUFFER_DEPTH:
      case __DRI_BUFFER_STENCIL:
      case __DRI_BUFFER_ACCUM:
      case __DRI_BUFFER_DEPTH_STENCIL:
      case __DRI_BUFFER_HIZ:
         local = yunos_alloc_local_buffer(dri2_surf,
               attachments[i], attachments[i + 1]);

         if (local) {
            *buf = *local;
            num_buffers++;
         }
         break;

      case __DRI_BUFFER_FRONT_LEFT:
      case __DRI_BUFFER_FRONT_RIGHT:
      case __DRI_BUFFER_FAKE_FRONT_LEFT:
      case __DRI_BUFFER_FAKE_FRONT_RIGHT:
      case __DRI_BUFFER_BACK_RIGHT:
      default:
         /* no front or right buffers */
         break;
      }
   }

   return num_buffers;
}


static _EGLSurface *
dri2_yunos_create_window_surface(_EGLDriver *drv, _EGLDisplay *disp,
                            _EGLConfig *conf, void *native_window,
                            const EGLint *attrib_list)
{
	_eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);

   return yunos_create_surface(drv, disp, EGL_WINDOW_BIT, conf,
                               native_window, attrib_list);
}

static _EGLSurface *
dri2_yunos_create_pbuffer_surface(_EGLDriver *drv, _EGLDisplay *disp,
			    _EGLConfig *conf, const EGLint *attrib_list)
{
	_eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);

   return yunos_create_surface(drv, disp, EGL_PBUFFER_BIT, conf,
			      NULL, attrib_list);
}

static _EGLSurface *
dri2_yunos_create_pixmap_surface(_EGLDriver *drv, _EGLDisplay *disp,
			    _EGLConfig *conf, void *native_pixmap, const EGLint *attrib_list)
{
	_eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);

   return dri2_fallback_create_pixmap_surface(drv, disp, conf, native_pixmap, attrib_list);
}


static EGLBoolean
dri2_yunos_destroy_surface(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf)
{
	_eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);


   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);

   yunos_free_local_buffers(dri2_surf);

   if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
      if (dri2_surf->buffer)
         yunos_surface_drop_buffer(disp, dri2_surf);

      dri2_surf->surface->base.decRef(&dri2_surf->surface->base);
   }

   if (dri2_surf->dri_image) {
       _eglLog(_EGL_DEBUG, "%s : %d : destroy dri_image", __func__, __LINE__);
      dri2_dpy->image->destroyImage(dri2_surf->dri_image);
      dri2_surf->dri_image = NULL;
   }

   if (dri2_surf->dri_image2) {
       _eglLog(_EGL_DEBUG, "%s : %d : destroy dri_image2", __func__, __LINE__);
      dri2_dpy->image->destroyImage(dri2_surf->dri_image2);
      dri2_surf->dri_image2 = NULL;
   }

   (*dri2_dpy->core->destroyDrawable)(dri2_surf->dri_drawable);

   dri2_fini_surface(surf);
   free(dri2_surf);

   return EGL_TRUE;
}


static int
dri2_yunos_image_get_buffers(__DRIdrawable *driDrawable,
                  unsigned int format,
                  uint32_t *stamp,
                  void *loaderPrivate,
                  uint32_t buffer_mask,
                  struct __DRIimageList *images)
{


   struct dri2_egl_surface *dri2_surf = loaderPrivate;

   images->image_mask = 0;
   images->front = NULL;
   images->back = NULL;
   _eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);
   if (yunos_update_buffers(dri2_surf) < 0)
      return 0;


   if (buffer_mask & __DRI_IMAGE_BUFFER_BACK) {
      if (yunos_get_back_bo(dri2_surf, format) < 0) {
         _eglError(EGL_BAD_PARAMETER, "get_back_bo");
         return 0;
      }

      images->back = dri2_surf->dri_image;
      images->image_mask |= __DRI_IMAGE_BUFFER_BACK;
   }


   if (buffer_mask & __DRI_IMAGE_BUFFER_FRONT) {
      if (yunos_get_front_bo(dri2_surf, format) < 0) {
         _eglError(EGL_BAD_PARAMETER, "get_front_bo");
         return 0;
      }

      images->front = dri2_surf->dri_image2;
      images->image_mask |= __DRI_IMAGE_BUFFER_FRONT;
   }

   return 1;
}

static EGLBoolean
dri2_yunos_swap_buffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw)
{
	_eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);

   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);

   if (dri2_surf->base.Type != EGL_WINDOW_BIT)
      return EGL_TRUE;

   dri2_flush_drawable_for_swapbuffers(disp, draw);

   if (dri2_surf->buffer)
      yunos_surface_submit_buffer(disp, dri2_surf);

   (*dri2_dpy->flush->invalidate)(dri2_surf->dri_drawable);

	/*
		BUG FIXME :
			submit producer buffer without fence
			then consumer may use the buffer before producer finish rendering
			if underlying dri driver & drm can't handle it correctly, it will cause tearing !!!
	*/


   return EGL_TRUE;
}

static _EGLImage *
dri2_create_image_yunos_native_buffer(_EGLDisplay *disp,
                                        _EGLContext *ctx,
                                        struct NativeSurfaceBuffer *buf)
{
	_eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);

   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img;
   int name, fd;
   int fourcc = 0;
   int offsets[YALLOC_MAX_PLANE_NUM] = {0};
   int pitches[YALLOC_MAX_PLANE_NUM] = {0};
   int plane_num = 0;
   int plane_index = 0;

   if (ctx != NULL) {
      _eglError(EGL_BAD_CONTEXT, "eglCreateEGLImageKHR: for "
                "EGL_NATIVE_BUFFER_YUNOS, the context must be "
                "EGL_NO_CONTEXT");
      return NULL;
   }

   if (!buf) {
      _eglError(EGL_BAD_PARAMETER, "eglCreateEGLImageKHR");
      return NULL;
   }

   fd = yunos_get_native_buffer_fd(dri2_dpy->yalloc, buf);
   _eglLog(_EGL_DEBUG, "%s:%d : fd = %d\n", __func__, __LINE__, fd);
   if (fd < 0) {
      name = yunos_get_native_buffer_name(dri2_dpy->yalloc, buf);
      _eglLog(_EGL_DEBUG, "%s:%d : name = %d\n", __func__, __LINE__, name);
      if (!name) {
 	       _eglLog(_EGL_WARNING, "%s:%d can't get fd or name from buffer !\n", __func__, __LINE__);
		   return NULL;
   	}
   }

   fourcc = yunos_get_fourcc(buf->format);
   plane_num = yunos_get_native_buffer_plane_num(dri2_dpy->yalloc, buf);
   if ((fourcc == -1) || (plane_num == 0)) {
      _eglLog(_EGL_DEBUG, "%s:%d : fourcc = %d, plane_num = %d\n", __func__, __LINE__, fourcc, plane_num);
      return NULL;
   }

   yunos_get_native_buffer_plane_offsets(dri2_dpy->yalloc, buf, offsets);
   yunos_get_native_buffer_plane_pitches(dri2_dpy->yalloc, buf, pitches);

	dri2_img = calloc(1, sizeof(*dri2_img));
	if (!dri2_img) {
		_eglError(EGL_BAD_ALLOC, "dri2_create_image_yunos_native_buffer : alloc dri2_img failed !");
		return NULL;
	}

	_eglInitImage(&dri2_img->base, disp);

   if(fd < 0) {
      dri2_img->dri_image =
         dri2_dpy->image->createImageFromNames(dri2_dpy->dri_screen,
                                                buf->width,
                                                buf->height,
                                                fourcc,
                                                &name,
                                                1,
                                                pitches,
                                                offsets,
                                                dri2_img);
   }
   else {
      dri2_img->dri_image =
         dri2_dpy->image->createImageFromFds(dri2_dpy->dri_screen,
                                             buf->width,
                                             buf->height,
                                             fourcc,
                                             &fd,
                                             1,
                                             pitches,
                                             offsets,
                                             dri2_img);
   }

	if (!dri2_img->dri_image) {
		free(dri2_img);
		_eglError(EGL_BAD_ALLOC, "dri2_create_image_yunos_native_buffer : createImageFromName failed !");
		return NULL;
	}

	return &dri2_img->base;
}

static _EGLImage *
dri2_yunos_create_image_khr(_EGLDriver *drv, _EGLDisplay *disp,
		       _EGLContext *ctx, EGLenum target,
		       EGLClientBuffer buffer, const EGLint *attr_list)
{
  _eglLog(_EGL_DEBUG, "%s:%d %s", __FILE__, __LINE__, __func__);
   switch (target) {
   case EGL_NATIVE_BUFFER_YUNOS:
#ifdef YUNOS_ENABLE_CNTR_CVG
   case EGL_NATIVE_BUFFER_ANDROID:
#endif
      return dri2_create_image_yunos_native_buffer(disp, ctx,
            (struct NativeSurfaceBuffer *) buffer);
   default:
      return dri2_create_image_khr(drv, disp, ctx, target, buffer, attr_list);
   }
}

static void
dri2_yunos_flush_front_buffer(__DRIdrawable * driDrawable, void *loaderPrivate)
{

	_eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);

	/* No fake front rendering now */
}

static __DRIbuffer *
dri2_yunos_get_buffers_with_format(__DRIdrawable * driDrawable,
			     int *width, int *height,
			     unsigned int *attachments, int count,
			     int *out_count, void *loaderPrivate)
{

   _eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);

   struct dri2_egl_surface *dri2_surf = loaderPrivate;

   if (yunos_update_buffers(dri2_surf) < 0)
      return NULL;

   _eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);

   *out_count =
      yunos_get_buffers_parse_attachments(dri2_surf, attachments, count);

   if (width)
      *width = dri2_surf->base.Width;
   if (height)
      *height = dri2_surf->base.Height;

   _eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);

   return dri2_surf->buffers;
}

static unsigned
dri2_yunos_get_capability(void *loaderPrivate, enum dri_loader_cap cap)
{
   /* Note: loaderPrivate is _EGLDisplay* */
   switch (cap) {
   case DRI_LOADER_CAP_RGBA_ORDERING:
      return 1;
   default:
      return 0;
   }
}

static EGLBoolean
dri2_yunos_add_configs_for_visuals(_EGLDriver *drv, _EGLDisplay *dpy)
{
	_eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);


   struct dri2_egl_display *dri2_dpy = dri2_egl_display(dpy);
   const struct {
      int format;
      unsigned int rgba_masks[4];
   } visuals[] = {
      { YUN_HAL_FORMAT_RGBA_8888, { 0xff, 0xff00, 0xff0000, 0xff000000 } },
      { YUN_HAL_FORMAT_RGBX_8888, { 0xff, 0xff00, 0xff0000, 0x0 } },
      { YUN_HAL_FORMAT_RGB_888,   { 0xff, 0xff00, 0xff0000, 0x0 } },
      { YUN_HAL_FORMAT_RGB_565,   { 0xf800, 0x7e0, 0x1f, 0x0 } },
      { YUN_HAL_FORMAT_BGRA_8888, { 0xff0000, 0xff00, 0xff, 0xff000000 } },
      { 0, { 0, 0, 0, 0 } }
   };

   EGLint config_attrs[] = {
      EGL_NATIVE_VISUAL_ID,   0,
      EGL_NATIVE_VISUAL_TYPE, 0,
      EGL_MAX_PBUFFER_WIDTH,  _EGL_MAX_PBUFFER_WIDTH,
      EGL_MAX_PBUFFER_HEIGHT, _EGL_MAX_PBUFFER_HEIGHT,
      EGL_NONE
   };
   int config_count, i, j;
   int srgb;

   config_count = 0;
   for (i = 0; visuals[i].format; i++) {
      int format_count = 0;

      config_attrs[1] = visuals[i].format;
      config_attrs[3] = visuals[i].format;

      for (j = 0; dri2_dpy->driver_configs[j]; j++) {
         const EGLint surface_type = EGL_WINDOW_BIT | EGL_PBUFFER_BIT;
         struct dri2_egl_config *dri2_conf;

         dri2_conf = dri2_add_config(dpy, dri2_dpy->driver_configs[j],
               config_count + 1, surface_type, config_attrs, visuals[i].rgba_masks);
         if (dri2_conf) {
            /* If the config id is the id passed in (config_count +1), it means a new config is added.
                       * Else, the config attributes just added to existing config.
                       * Only increase count id count, aks config id,  in new config added case.
                       */
            if (dri2_conf->base.ConfigID == (config_count + 1)) {
               config_count++;
            }
            format_count++;
         }
      }

      if (!format_count) {
         _eglLog(_EGL_DEBUG, "No DRI config supports native format 0x%x",
               visuals[i].format);
      }
   }

   /* post-process configs */
   for (i = 0; i < dpy->Configs->Size; i++) {
      struct dri2_egl_config *dri2_conf = dri2_egl_config(dpy->Configs->Elements[i]);

      dri2_conf->base.RenderableType &= ~EGL_OPENGL_BIT;
      dri2_conf->base.Conformant &= ~EGL_OPENGL_BIT;

      for (srgb = 0; srgb < 2; srgb++) {
         if (!dri2_conf->dri_config[0][srgb] && !dri2_conf->dri_config[1][srgb])
            continue;
         if (!dri2_conf->dri_config[0][srgb])
            dri2_conf->base.SurfaceType &= ~EGL_PBUFFER_BIT;
         else if (!dri2_conf->dri_config[1][srgb])
            dri2_conf->base.SurfaceType &= ~EGL_WINDOW_BIT;
      }
   }

   return (config_count != 0);
}

static inline EGLBoolean dri2_fallback_swap_interval(_EGLDriver *drv, _EGLDisplay *dpy,
  _EGLSurface *surf, EGLint interval)
{
  _eglLog(_EGL_DEBUG, "%s:%d %s", __FILE__, __LINE__, __func__);
  return EGL_FALSE;
}

static struct dri2_egl_display_vtbl yunos_display_vtbl = {
   .authenticate = NULL,
   .create_window_surface = dri2_yunos_create_window_surface,
   .create_pixmap_surface = dri2_yunos_create_pixmap_surface,
   .create_pbuffer_surface = dri2_yunos_create_pbuffer_surface,
   .destroy_surface = dri2_yunos_destroy_surface,
   .create_image = dri2_yunos_create_image_khr,
   .swap_interval = dri2_fallback_swap_interval,
   .swap_buffers = dri2_yunos_swap_buffers,
   .swap_buffers_with_damage = dri2_fallback_swap_buffers_with_damage,
   .swap_buffers_region = dri2_fallback_swap_buffers_region,
   .post_sub_buffer = dri2_fallback_post_sub_buffer,
   .copy_buffers = dri2_fallback_copy_buffers,
   .query_buffer_age = dri2_fallback_query_buffer_age,
   .create_wayland_buffer_from_image = dri2_fallback_create_wayland_buffer_from_image,
   .get_sync_values = dri2_fallback_get_sync_values,
   .get_dri_drawable = dri2_surface_get_dri_drawable,
};

static const __DRIdri2LoaderExtension yunos_dri2_loader_extension = {
  .base = { __DRI_DRI2_LOADER, 4 },

  .getBuffers			 = NULL,
  .flushFrontBuffer	 = dri2_yunos_flush_front_buffer,
  .getBuffersWithFormat = dri2_yunos_get_buffers_with_format,
  .getCapability       = dri2_yunos_get_capability,
};

static const __DRIimageLoaderExtension yunos_image_loader_extension = {
   .base = { __DRI_IMAGE_LOADER, 2 },

   .getBuffers          = dri2_yunos_image_get_buffers,
   .flushFrontBuffer    = dri2_yunos_flush_front_buffer,
   .getCapability       = dri2_yunos_get_capability,
};

static const __DRIextension *yunos_dri2_loader_extensions[] = {
  &yunos_dri2_loader_extension.base,
  &use_invalidate.base,
  &image_lookup_extension.base,
  NULL,
};

static const __DRIextension *yunos_image_loader_extensions[] = {
  &yunos_image_loader_extension.base,
  &use_invalidate.base,
  &image_lookup_extension.base,
  NULL,
};


EGLBoolean
dri2_initialize_yunos(_EGLDriver *drv, _EGLDisplay *dpy)
{
   struct dri2_egl_display *dri2_dpy;
   const char *err;

//   _eglSetLogProc(yunos_log);

   loader_set_logger(_eglLog);

   _eglLog(_EGL_DEBUG, "%s:%d\n", __func__, __LINE__);

   dri2_dpy = calloc(1, sizeof(*dri2_dpy));
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   dpy->DriverData = (void *) dri2_dpy;

   if(!yunos_open_device(dri2_dpy)) {
      err = "DRI2: failed to open device";
      goto cleanup;
   }

   dri2_dpy->driver_name = loader_get_driver_for_fd(dri2_dpy->fd);
   if (dri2_dpy->driver_name == NULL) {
      err = "DRI2: failed to get driver name";
      goto cleanup;
   }

   if (!dri2_load_driver(dpy)) {
      err = "DRI2: failed to load driver";
      goto cleanup;
   }

   dri2_dpy->is_render_node = drmGetNodeTypeFromFd(dri2_dpy->fd) == DRM_NODE_RENDER;

   /* render nodes cannot use Gem names, and thus do not support
       * the __DRI_DRI2_LOADER extension */
   if (!dri2_dpy->is_render_node && !dri2_dpy->is_buf_prime_fd) {
	  dri2_dpy->loader_extensions = yunos_image_loader_extensions;
	  if (!dri2_load_driver(dpy)) {
         err = "DRI2: failed to load driver";
         goto cleanup;
      }
   } else {
      dri2_dpy->loader_extensions = yunos_image_loader_extensions;
	  if (!dri2_load_driver_dri3(dpy)) {
         err = "DRI3: failed to load driver";
         goto cleanup;
      }
   }

   if (!dri2_create_screen(dpy)) {
      err = "DRI2: failed to create screen";
      goto cleanup;
   }

   if (!dri2_setup_extensions(dpy)) {
      err = "DRI2: failed to setup extensions";
      goto cleanup;
   }

   dri2_setup_screen(dpy);

   if (!dri2_yunos_add_configs_for_visuals(drv, dpy)) {
      err = "DRI2: failed to add configs";
      goto cleanup;
   }

   // FIXME : YunOS needs an image native buffer ext like : YOS_image_native_buffer;
   dpy->Extensions.KHR_image_base = EGL_TRUE;
   dpy->Extensions.KHR_image = EGL_TRUE;
   dpy->Extensions.EXT_buffer_age = EGL_TRUE;

   /* Fill vtbl */
   dri2_dpy->vtbl = &yunos_display_vtbl;
   return EGL_TRUE;

cleanup:
   dri2_display_destroy(dpy);

   return _eglError(EGL_NOT_INITIALIZED, err);
}
