/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "anv_private.h"
#include "vk_enum_to_str.h"

/** Log an error message.  */
void anv_printflike(1, 2)
anv_loge(const char *format, ...)
{
   va_list va;

   va_start(va, format);
   anv_loge_v(format, va);
   va_end(va);
}

/** \see anv_loge() */
void
anv_loge_v(const char *format, va_list va)
{
   intel_loge_v(format, va);
}

void anv_printflike(6, 7)
__anv_perf_warn(struct anv_instance *instance, const void *object,
                VkDebugReportObjectTypeEXT type,
                const char *file, int line, const char *format, ...)
{
   va_list ap;
   char buffer[256];
   char report[512];

   va_start(ap, format);
   vsnprintf(buffer, sizeof(buffer), format, ap);
   va_end(ap);

   snprintf(report, sizeof(report), "%s: %s", file, buffer);

   vk_debug_report(&instance->debug_report_callbacks,
                   VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                   type,
                   (uint64_t) (uintptr_t) object,
                   line,
                   0,
                   "anv",
                   report);

   intel_logw("%s:%d: PERF: %s", file, line, buffer);
}

const char *
vk_Result_to_str_pri(VkResult input)
{
    switch(input) {
        case -1000244000:
            return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
        case -1000174001:
            return "VK_ERROR_NOT_PERMITTED_EXT";
        case -1000161000:
            return "VK_ERROR_FRAGMENTATION_EXT";
        case -1000158000:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case -1000072003:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case -1000069000:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case -1000012000:
            return "VK_ERROR_INVALID_SHADER_NV";
        case -1000011001:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case -1000003001:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case -1000001004:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case -1000000001:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case -1000000000:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case -12:
            return "VK_ERROR_FRAGMENTED_POOL";
        case -11:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case -10:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case -9:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case -8:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case -7:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case -6:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case -5:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case -4:
            return "VK_ERROR_DEVICE_LOST";
        case -3:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case -2:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case -1:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case 0:
            return "VK_SUCCESS";
        case 1:
            return "VK_NOT_READY";
        case 2:
            return "VK_TIMEOUT";
        case 3:
            return "VK_EVENT_SET";
        case 4:
            return "VK_EVENT_RESET";
        case 5:
            return "VK_INCOMPLETE";
        case 1000001003:
            return "VK_SUBOPTIMAL_KHR";
    default:
        unreachable("Undefined enum value.");
    }
}

VkResult
__vk_errorv(struct anv_instance *instance, const void *object,
            VkDebugReportObjectTypeEXT type, VkResult error,
            const char *file, int line, const char *format, va_list ap)
{
   char buffer[256];
   char report[512];

   const char *error_str = vk_Result_to_str_pri(error);

   if (format) {
      vsnprintf(buffer, sizeof(buffer), format, ap);

      snprintf(report, sizeof(report), "%s:%d: %s (%s)", file, line, buffer,
               error_str);
   } else {
      snprintf(report, sizeof(report), "%s:%d: %s", file, line, error_str);
   }

   if (instance) {
      vk_debug_report(&instance->debug_report_callbacks,
                      VK_DEBUG_REPORT_ERROR_BIT_EXT,
                      type,
                      (uint64_t) (uintptr_t) object,
                      line,
                      0,
                      "anv",
                      report);
   }

   intel_loge("%s", report);

   return error;
}

VkResult
__vk_errorf(struct anv_instance *instance, const void *object,
            VkDebugReportObjectTypeEXT type, VkResult error,
            const char *file, int line, const char *format, ...)
{
   va_list ap;

   va_start(ap, format);
   __vk_errorv(instance, object, type, error, file, line, format, ap);
   va_end(ap);

   return error;
}
