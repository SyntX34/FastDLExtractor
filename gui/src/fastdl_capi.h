#pragma once

/*
 * FastDL Tool  --  C API Bridge
 * Exposes the C++ backend as a plain-C shared library so Python (ctypes)
 * can call it on Linux / Windows / macOS without a C++ runtime dependency
 * in the Python layer.
 *
 * All strings are UTF-8 null-terminated.
 */

#ifdef _WIN32
#  define FDLAPI __declspec(dllexport)
#else
#  define FDLAPI __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ─── opaque handle ──────────────────────────────────────────────────────── */
typedef void* FDLHandle;

/* ─── progress callback (called from worker threads) ─────────────────────── */
typedef void (*FDLProgressCB)(
    const char* filename,   /* relative path                              */
    double      progress,   /* 0.0 – 1.0 for this file                   */
    long long   downloaded, /* bytes so far for this file                 */
    long long   total,      /* total bytes for this file (0 = unknown)    */
    double      speed,      /* bytes/second                               */
    double      eta,        /* seconds remaining (-1 = unknown)           */
    int         done_files, /* cumulative files finished                  */
    long long   done_bytes, /* cumulative bytes finished                  */
    void*       userdata
);

typedef void (*FDLEventCB)(
    int         event,      /* 0=file_start 1=file_done 2=extract_start   */
                            /* 3=extract_done 4=error 5=scan_result        */
    const char* filename,
    const char* detail,     /* error text, or NULL                        */
    long long   size,
    void*       userdata
);

/* ─── lifecycle ──────────────────────────────────────────────────────────── */
FDLAPI FDLHandle fdl_create(
    const char* base_url,
    const char* output_dir,
    int         num_threads
);

FDLAPI void fdl_destroy(FDLHandle h);

/* ─── configuration ──────────────────────────────────────────────────────── */
FDLAPI void fdl_set_resource_types(FDLHandle h,
                                   const char** types,
                                   int          count);

FDLAPI void fdl_set_max_retries(FDLHandle h, int retries);
FDLAPI void fdl_set_timeout(FDLHandle h,    int seconds);

/* ─── callbacks (thread-safe; called from worker threads) ────────────────── */
FDLAPI void fdl_set_progress_cb(FDLHandle h, FDLProgressCB cb, void* userdata);
FDLAPI void fdl_set_event_cb   (FDLHandle h, FDLEventCB    cb, void* userdata);

/* ─── operations ─────────────────────────────────────────────────────────── */

/* Queue a single relative file for download. Returns 1 on success. */
FDLAPI int  fdl_download_file(FDLHandle h, const char* relative_path);

/* Crawl a remote directory and return newline-separated filenames.
 * Caller must free() the returned buffer.  Returns NULL on error. */
FDLAPI char* fdl_fetch_listing(FDLHandle h, const char* relative_dir);

/* Block until all queued downloads finish. */
FDLAPI void  fdl_wait_all(FDLHandle h);

/* Cancel all queued downloads. */
FDLAPI void  fdl_cancel(FDLHandle h);

/* ─── stats ──────────────────────────────────────────────────────────────── */
FDLAPI long long fdl_total_bytes(FDLHandle h);
FDLAPI long long fdl_total_files(FDLHandle h);

/* ─── config file helpers (thin wrappers around ConfigManager) ───────────── */

/* Returns JSON string (caller must free()).  NULL on error. */
FDLAPI char* fdl_config_load(const char* filepath);

/* Returns 1 on success. */
FDLAPI int   fdl_config_save(const char* filepath, const char* json_content);

/* ─── utility ────────────────────────────────────────────────────────────── */
FDLAPI const char* fdl_version(void);

#ifdef __cplusplus
} /* extern "C" */
#endif