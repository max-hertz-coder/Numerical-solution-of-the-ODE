#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    const char *outdir = (argc > 1) ? argv[1] : "out";
    char p1[256], p2[256], ps[256];

    long double T, hcrit, hS, h_usedS, hU, h_usedU;
    size_t nS, nU;

    long double t, xe, xt, xx, ae, at;

    long double s_ae, s_tae, s_at, s_tat, s_re, s_rt;
    long double u_ae, u_tae, u_at, u_tat, u_re, u_rt;

    FILE *fs, *fu, *fsum;
    size_t i;

    snprintf(p1, sizeof(p1), "%s/stable.tsv", outdir);
    snprintf(p2, sizeof(p2), "%s/unstable.tsv", outdir);
    snprintf(ps, sizeof(ps), "%s/summary.txt", outdir);

    fs = fopen(p1, "w");
    fu = fopen(p2, "w");
    fsum = fopen(ps, "w");
    if (!fs || !fu || !fsum) exit(1);

    if (scanf(" %Lf %Lf %Lf %zu %Lf %Lf %zu %Lf",
              &T, &hcrit, &hS, &nS, &h_usedS, &hU, &nU, &h_usedU) != 8) exit(1);

    fprintf(fs, "# Вариант 1: x'=-2 t x, x(0)=1, x_точн=exp(-t^2)\n");
    fprintf(fs, "# 1:t 2:x_euler 3:x_trap 4:x_exact 5:abs_err_euler 6:abs_err_trap\n");
    for (i = 0; i <= nS; i++) {
        scanf(" %Lf %Lf %Lf %Lf %Lf %Lf", &t, &xe, &xt, &xx, &ae, &at);
        fprintf(fs, "%.17Lg\t%.17Lg\t%.17Lg\t%.17Lg\t%.17Lg\t%.17Lg\n", t, xe, xt, xx, ae, at);
    }

    fprintf(fu, "# Вариант 1: x'=-2 t x, x(0)=1, x_точн=exp(-t^2)\n");
    fprintf(fu, "# 1:t 2:x_euler 3:x_trap 4:x_exact 5:abs_err_euler 6:abs_err_trap\n");
    for (i = 0; i <= nU; i++) {
        scanf(" %Lf %Lf %Lf %Lf %Lf %Lf", &t, &xe, &xt, &xx, &ae, &at);
        fprintf(fu, "%.17Lg\t%.17Lg\t%.17Lg\t%.17Lg\t%.17Lg\t%.17Lg\n", t, xe, xt, xx, ae, at);
    }

    scanf(" %Lf %Lf %Lf %Lf %Lf %Lf", &s_ae, &s_tae, &s_at, &s_tat, &s_re, &s_rt);
    scanf(" %Lf %Lf %Lf %Lf %Lf %Lf", &u_ae, &u_tae, &u_at, &u_tat, &u_re, &u_rt);

    fprintf(fsum, "Вариант 1: x'=-2 t x, x(0)=1, x_точн=exp(-t^2)\n");
    fprintf(fsum, "T = %.10Lg\n", T);
    fprintf(fsum, "h_crit = %.10Lg\n", hcrit);

    fprintf(fsum, "\nУстойчивый прогон:\n");
    fprintf(fsum, "  h = %.10Lg, n = %zu, h_used = %.10Lg\n", hS, nS, h_usedS);
    fprintf(fsum, "  max_abs_err(Эйлер) = %.6Le при t=%.6Lg\n", s_ae, s_tae);
    fprintf(fsum, "  max_abs_err(Неявн)  = %.6Le при t=%.6Lg\n", s_at, s_tat);

    fprintf(fsum, "\nНеустойчивый прогон:\n");
    fprintf(fsum, "  h = %.10Lg, n = %zu, h_used = %.10Lg\n", hU, nU, h_usedU);
    fprintf(fsum, "  max_abs_err(Эйлер) = %.6Le при t=%.6Lg\n", u_ae, u_tae);
    fprintf(fsum, "  max_abs_err(Неявн)  = %.6Le при t=%.6Lg\n", u_at, u_tat);

    fclose(fs);
    fclose(fu);
    fclose(fsum);
    return 0;
}
