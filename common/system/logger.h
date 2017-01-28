#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void log_init(const char *ipString, int port);
void log_deinit(void);

#ifdef __cplusplus
}
#endif


