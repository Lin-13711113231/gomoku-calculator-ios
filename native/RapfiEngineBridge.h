#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*RapfiLineCallback)(const char *line, void *context);

int rapfi_engine_start(const char *config_path, RapfiLineCallback callback, void *context);
void rapfi_engine_send(const char *command);
void rapfi_engine_stop(void);

#ifdef __cplusplus
}
#endif
