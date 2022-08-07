/*
 * Copyright (C) 2018, Emilio G. Cota <cota@braap.org>
 *
 * License: GNU GPL, version 2 or later.
 *   See the COPYING file in the top-level directory.
 */
#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <glib.h>

#include <qemu-plugin.h>

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

#define EXCEPTION_MAX 20
#define VCPU_MAX 4

static time_t sample_period_s = 1;
static time_t last_sample;
static int32_t exception_counts[VCPU_MAX][EXCEPTION_MAX];
static FILE* tracefile;

static void plugin_exit(qemu_plugin_id_t id, void *p)
{
    if (tracefile != stdout) {
        fclose(tracefile);
    }
}

static void record_samples(void)
{
    time_t now = time(NULL);
    char tm[128];

    if ((now - last_sample) >= sample_period_s) {
        strftime(tm, sizeof(tm), "%Y-%m-%dT%T", localtime(&now));

        for (int c = 0; c < VCPU_MAX; c++) {
            fprintf(tracefile, "%s%4d", tm, c);
            for (int i = 0; i < EXCEPTION_MAX; i++) {
                fprintf(tracefile, " %8d", exception_counts[c][i]);
                exception_counts[c][i] = 0;
            }
            fprintf(tracefile, "\n");
        }

        last_sample = now;
    }
}

static void vcpu_exception(qemu_plugin_id_t id, unsigned int vcpu_index,
                           int idx)
{
    if (vcpu_index >= VCPU_MAX) {
        fprintf(stderr, "CPU %d: CPU index %d is larger than max %d\n",
                vcpu_index, vcpu_index, VCPU_MAX);
        return;
    }
    if (idx >= EXCEPTION_MAX) {
        fprintf(stderr, "CPU %d: Exception index %d is larger than max %d\n",
                vcpu_index, idx, EXCEPTION_MAX);
        return;
    }
    exception_counts[vcpu_index][idx]++;

    record_samples();
}

// target long
// arg=tracepath,arg=/some/path/irqs.txt
QEMU_PLUGIN_EXPORT int qemu_plugin_install(qemu_plugin_id_t id,
                                           const qemu_info_t *info,
                                           int argc, char **argv)
{
    char *tracepath = NULL;
    int c;

    for (c = 0; c < argc; c++) {
        char *opt = argv[c];
        g_autofree char **tokens = g_strsplit(opt, "=", 2);

        if (g_strcmp0(tokens[0], "tracepath") == 0) {
            c++;
            tracepath = strdup(tokens[1]);
        } else {
            fprintf(stderr, "Unknown argument: %d %s\n", c, argv[c]);
        }
    }

    /* Setup Tracefile */
    if (tracepath == NULL) {
        tracepath = strdup("irqs.txt");
    }

    if (tracepath[0] == '-') {
        tracefile = stdout;
    } else {
        tracefile = fopen(tracepath, "w");
    }

    free(tracepath);

    fprintf(tracefile, "                     CS             RESET   BUSERR      DPF      IPF     TICK    ALIGN      ILL      INT     DTLB     ITLB     RANG  SYSCALL\n");

    qemu_plugin_register_vcpu_exception_cb(id, vcpu_exception);
    qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);

    last_sample = time(NULL);

    return 0;
}
