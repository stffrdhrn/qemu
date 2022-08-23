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
#include <glib.h>

#include <qemu-plugin.h>

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

#define VCPU_MAX 4

static time_t sample_period_s = 1;
static time_t last_sample;
static uint64_t mem_counts[VCPU_MAX];
static uint64_t io_counts[VCPU_MAX];

static enum qemu_plugin_mem_rw rw = QEMU_PLUGIN_MEM_RW;
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

            fprintf(tracefile, " %8ld", mem_counts[c]);
            mem_counts[c] = 0;

            fprintf(tracefile, " %8ld", io_counts[c]);
            io_counts[c] = 0;

            fprintf(tracefile, "\n");
        }

        last_sample = now;
    }
}

static void vcpu_mem(unsigned int cpu_index, qemu_plugin_meminfo_t meminfo,
                     uint64_t vaddr, void *udata)
{
    struct qemu_plugin_hwaddr *hwaddr;

    if (cpu_index >= VCPU_MAX) {
        fprintf(stderr, "CPU %d: CPU index %d is larger than max %d\n",
                cpu_index, cpu_index, VCPU_MAX);
        return;
    }

    hwaddr = qemu_plugin_get_hwaddr(meminfo, vaddr);
    if (qemu_plugin_hwaddr_is_io(hwaddr)) {
        io_counts[cpu_index]++;
    } else {
        mem_counts[cpu_index]++;
    }

    record_samples();
}

static void vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    size_t n = qemu_plugin_tb_n_insns(tb);
    size_t i;

    for (i = 0; i < n; i++) {
        struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);

        qemu_plugin_register_vcpu_mem_cb(insn, vcpu_mem,
                                         QEMU_PLUGIN_CB_NO_REGS,
                                         rw, NULL);
    }
}

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
        tracepath = strdup("mem.txt");
    }

    if (tracepath[0] == '-') {
        tracefile = stdout;
    } else {
        tracefile = fopen(tracepath, "w");
    }

    free(tracepath);

    qemu_plugin_register_vcpu_tb_trans_cb(id, vcpu_tb_trans);
    qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);
    return 0;
}
