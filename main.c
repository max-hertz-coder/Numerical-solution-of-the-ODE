#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdarg.h>

typedef struct {
    long double max_abs_e, t_max_abs_e;
    long double max_abs_t, t_max_abs_t;
    long double max_rel_e, max_rel_t;
} Metrics;

static void w_all(int fd, const void *buf, size_t n)
{
    const char *p = (const char *)buf;
    while (n) {
        ssize_t w = write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR) continue;
            _exit(1);
        }
        p += (size_t)w;
        n -= (size_t)w;
    }
}

static void wfmt(int fd, const char *fmt, ...)
{
    char b[512];
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(b, sizeof(b), fmt, ap);
    va_end(ap);
    if (n < 0) {
        _exit(1);
    }
    if ((size_t)n > sizeof(b)) {
        n = (int)sizeof(b);
    }
    w_all(fd, b, (size_t)n);
}

static void run_send(int fd, size_t n, long double h, Metrics *m)
{
    size_t i;
    long double x_e = 1.0L, x_t = 1.0L;

    memset(m, 0, sizeof(*m));

    for (i = 0; i <= n; i++) {
        long double t = (long double)i * h;
        long double x_ex = expl(-t * t);

        long double ae = fabsl(x_e - x_ex);
        long double at = fabsl(x_t - x_ex);

        long double den = fabsl(x_ex);
        if (den < 1e-300L) den = 1e-300L;

        long double re = ae / den;
        long double rt = at / den;

        if (ae > m->max_abs_e) {
            m->max_abs_e = ae;
            m->t_max_abs_e = t;
        }
        if (at > m->max_abs_t) {
            m->max_abs_t = at;
            m->t_max_abs_t = t;
        }
        if (re > m->max_rel_e) {
            m->max_rel_e = re;
        }
        if (rt > m->max_rel_t) {
            m->max_rel_t = rt;
        }

        wfmt(fd, "%.20Lg %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg\n",
             t, x_e, x_t, x_ex, ae, at);

        if (i == n) break;

        long double t_n = t;
        long double t_np1 = t + h;

        x_e = x_e * (1.0L - 2.0L * h * t_n);
        x_t = x_t * (1.0L - h * t_n) / (1.0L + h * t_np1);
    }
}

static pid_t son(const char *prog, const char *arg1, int *wfd)
{
    int p[2];
    pid_t pid;

    if (pipe(p) < 0) {
        _exit(1);
    }
    pid = fork();
    if (pid < 0) {
        _exit(1);
    }

    if (pid == 0) {
        if (dup2(p[0], STDIN_FILENO) < 0) _exit(1);
        close(p[0]);
        close(p[1]);
        execl(prog, prog, arg1, (char *)NULL);
        _exit(1);
    }

    close(p[0]);
    *wfd = p[1];
    return pid;
}

static void wait_one(void)
{
    while (wait(NULL) < 0 && errno == EINTR) {}
}

static void print_help(const char *prog)
{
    printf("Численное исследование устойчивости схемы Эйлера (вариант 1)\n\n");
    printf("Задача Коши:\n");
    printf("  x' = -2 t x,   x(0)=1,   x_точн = exp(-t^2)\n\n");
    printf("Ключи:\n");
    printf("  -H            справка\n");
    printf("  -T   T        конечное время (интервал [0, T])\n");
    printf("  -HS  h        шаг устойчивого прогона (если не задан — авто)\n");
    printf("  -HU  h        шаг неустойчивого прогона (если не задан — авто)\n");
    printf("  -O   dir      папка вывода (по умолчанию: out; папка должна существовать)\n");
    printf("  -NP           не строить графики (не запускать plot)\n");
    printf("  -Q            меньше вывода (в этой версии main почти не печатает)\n\n");
    printf("Пример:\n");
    printf("  %s -T 30 -HS 0.02 -HU 2.02 -O out\n", prog);
}

int main(int argc, char **argv)
{
    long double T = 30.0L, hS = 0.0L, hU = 0.0L;
    char hs_set = 0, hu_set = 0;
    const char *outdir = "out";
    char no_plot = 0;
    int i;

    if (argc == 1) { print_help(argv[0]); return 0; }

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-H")) {
            print_help(argv[0]);
            return 0;
        }
        else if (!strcmp(argv[i], "-NP")) {
            no_plot = 1;
        }
        else if (!strcmp(argv[i], "-T")) {
            if (++i >= argc) exit(1);
            sscanf(argv[i], " %Lf", &T);
        }
        else if (!strcmp(argv[i], "-HS")) {
            if (++i >= argc) exit(1);
            sscanf(argv[i], " %Lf", &hS);
            hs_set = 1;
        }
        else if (!strcmp(argv[i], "-HU")) {
            if (++i >= argc) exit(1);
            sscanf(argv[i], " %Lf", &hU);
            hu_set = 1;
        }
        else if (!strcmp(argv[i], "-O")) {
            if (++i >= argc) exit(1);
            outdir = argv[i];
        }
        else exit(1);
    }

    long double hcrit = 1.0L / T;
    if (!hs_set) {
        hS = 0.8L * hcrit;
    }
    if (!hu_set) {
        hU = 1.05L * hcrit;
    }

    size_t nS = (size_t)ceill(T / hS);
    if (nS < 1) nS = 1;
    size_t nU = (size_t)ceill(T / hU);
    if (nU < 1) nU = 1;

    long double h_usedS = T / (long double)nS;
    long double h_usedU = T / (long double)nU;

    Metrics ms, mu;

    int wfd;
    pid_t pid = son("./table", outdir, &wfd);

    wfmt(wfd, "%.20Lg %.20Lg %.20Lg %zu %.20Lg %.20Lg %zu %.20Lg\n",
         T, hcrit, hS, nS, h_usedS, hU, nU, h_usedU);

    run_send(wfd, nS, h_usedS, &ms);
    run_send(wfd, nU, h_usedU, &mu);

    wfmt(wfd, "%.20Lg %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg\n",
         ms.max_abs_e, ms.t_max_abs_e, ms.max_abs_t, ms.t_max_abs_t, ms.max_rel_e, ms.max_rel_t);
    wfmt(wfd, "%.20Lg %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg\n",
         mu.max_abs_e, mu.t_max_abs_e, mu.max_abs_t, mu.t_max_abs_t, mu.max_rel_e, mu.max_rel_t);

    close(wfd);
    wait_one();
    (void)pid;

    if (!no_plot) {
        int wfd2;
        son("./plot", outdir, &wfd2);
        wfmt(wfd2, "0\n");
        close(wfd2);
        wait_one();
    }

    return 0;
}
