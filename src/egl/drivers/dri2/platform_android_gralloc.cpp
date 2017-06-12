#include "egl_dri2.h"

extern "C" int update_pfn(struct dri2_egl_display *dri2_dpy);
int update_pfn(struct dri2_egl_display *dri2_dpy){
   const char *err;
   int ret;
   //const hw_module_t *gralloc1 ;
   hw_device_t *device;

   ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
                       (const hw_module_t **)&dri2_dpy->gralloc);
   if (ret) {
      //err = "DRI2: failed to get gralloc module";
   }
//   gralloc1 = reinterpret_cast<hw_module_t *>(dri2_dpy->gralloc);
   dri2_dpy->gralloc_version = dri2_dpy->gralloc->module_api_version;

     if(dri2_dpy->gralloc_version == HARDWARE_MODULE_API_VERSION(1, 0)){
   ret = dri2_dpy->gralloc->methods->open(dri2_dpy->gralloc, GRALLOC_HARDWARE_MODULE_ID, &device);
   if (ret) {
     // err = "Failed to open hw_device device";
   }
   else {
  
     dri2_dpy->gralloc1_dvc = reinterpret_cast<gralloc1_device_t *>(device);

     dri2_dpy->pfn_lockflex = reinterpret_cast<GRALLOC1_PFN_LOCK_FLEX>\
               (dri2_dpy->gralloc1_dvc->getFunction(dri2_dpy->gralloc1_dvc, GRALLOC1_FUNCTION_LOCK_FLEX)); 
     }
   }
   return ret;
}

