#ifndef DUMP_BATCH_BUFFER_H
#define DUMP_BATCH_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif
extern FILE *create_log_file();

extern void close_log_file(FILE **file);

#ifdef __cplusplus
}
#endif

#endif /* DUMP_BATCH_BUFFER_H */
