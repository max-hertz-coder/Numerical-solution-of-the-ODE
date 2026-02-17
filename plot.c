#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

static void run_gp(const char *gp_path)
{
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid == 0) {
        int dn = open("/dev/null", O_WRONLY);
        if (dn >= 0) {
            dup2(dn, STDOUT_FILENO);
            dup2(dn, STDERR_FILENO);
            close(dn);
        }
        execlp("gnuplot", "gnuplot", gp_path, (char *)NULL);
        _exit(1);
    }
    while (wait(NULL) < 0 && errno == EINTR) {}
}

static void write_gp(const char *gp_path, const char *png_path, const char *tsv_path, int unstable)
{
    FILE *f = fopen(gp_path, "w");
    if (!f) exit(1);

    fprintf(f, "set encoding utf8\n");
    fprintf(f, "set term pngcairo size 900,600 font 'DejaVu Sans,12'\n");
    fprintf(f, "set output '%s'\n", png_path);
    fprintf(f, "unset title\n");
    fprintf(f, "set grid\n");
    fprintf(f, "set key left top\n");
    fprintf(f, "set xlabel 't'\n");
    fprintf(f, "set ylabel 'x'\n");
    fprintf(f, "set y2label 'ошибка |x_{num} - x_{точн}|'\n");
    fprintf(f, "set y2tics\n");
    fprintf(f, "set logscale y2\n");
    fprintf(f, "set format y2 '10^{%%T}'\n");

    if (unstable) {
        fprintf(f, "set xrange [0:5]\n");
        fprintf(f, "set yrange [-10:10]\n");
        fprintf(f, "set y2range [1e-10:100]\n");
    } else {
        fprintf(f, "set yrange [0:1.1]\n");
        fprintf(f, "set y2range [1e-10:10]\n");
    }

    fprintf(f,
        "plot '%s' using 1:2 with points pt 3 ps 0.9 lc rgb 'red' title 'схема Эйлера',\\\n"
        "     ''  using 1:3 with points pt 7 ps 0.9 lc rgb 'blue' title 'неявная схема',\\\n"
        "     ''  using 1:4 with lines lw 2 lc rgb 'black' title 'аналитическое решение',\\\n"
        "     ''  using 1:(($5<1e-16)?1e-16:$5) axes x1y2 with lines lw 2 lc rgb 'red' title 'ошибка схемы Эйлера',\\\n"
        "     ''  using 1:(($6<1e-16)?1e-16:$6) axes x1y2 with lines lw 2 lc rgb 'blue' title 'ошибка неявной схемы'\n",
        tsv_path
    );

    fclose(f);
}

int main(int argc, char **argv)
{
    const char *outdir = (argc > 1) ? argv[1] : "out";
    char gp1[256], gp2[256], png1[256], png2[256], tsv1[256], tsv2[256];
    int dummy;

    scanf(" %d", &dummy);

    snprintf(gp1, sizeof(gp1), "%s/stable.gp", outdir);
    snprintf(gp2, sizeof(gp2), "%s/unstable.gp", outdir);
    snprintf(png1, sizeof(png1), "%s/stable.png", outdir);
    snprintf(png2, sizeof(png2), "%s/unstable.png", outdir);
    snprintf(tsv1, sizeof(tsv1), "%s/stable.tsv", outdir);
    snprintf(tsv2, sizeof(tsv2), "%s/unstable.tsv", outdir);

    write_gp(gp1, png1, tsv1, 0);
    write_gp(gp2, png2, tsv2, 1);

    run_gp(gp1);
    run_gp(gp2);
    return 0;
}
