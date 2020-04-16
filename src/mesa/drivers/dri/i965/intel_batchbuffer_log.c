#include "intel_batchbuffer_log.h"

FILE *create_log_file()
{
   char file_name[1000]= {0};
   char dir_name[1000] = "/data/data/android/mesa3d_intel";
   time_t timep;
   struct tm *p;
   int tid;
   time(&timep);
   p = gmtime(&timep);
   FILE *f = NULL;

   tid = gettid();

   snprintf(file_name, 100, "/data/data/android/mesa3d_intel/%04d%02d%02d_%02d%02d%02d_%u_mesa_batch.log", 1900+p->tm_year, p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, tid);
   f = fopen(file_name, "a+");

   dbg_printf("create log file %p %s, %p", p, file_name, f);
   //dumping_callstack();
   if (!f) {
      dbg_printf("create file fail errno = %d reason = %s \n", errno, strerror(errno));
   }
   return f;
}

void close_log_file(FILE *file)
{
   if (file != NULL && file != stderr)
   {
      dbg_printf("close log file");
      fclose(file);
   }
}
