#include "ordenacao.h"
#include <stdlib.h>
#include <string.h>
static void ordenar(Registro *v, Registro *aux, size_t inicio, size_t fim) {
    size_t meio, i, j, k;
    if (fim-inicio<2) return;
    meio=inicio+(fim-inicio)/2;
    ordenar(v,aux,inicio,meio); ordenar(v,aux,meio,fim);
    i=inicio; j=meio; k=inicio;
    while (i<meio && j<fim) aux[k++]=v[i].timestamp<=v[j].timestamp ? v[i++] : v[j++];
    while (i<meio) aux[k++]=v[i++];
    while (j<fim) aux[k++]=v[j++];
    memcpy(v+inicio,aux+inicio,(fim-inicio)*sizeof *v);
}
int mergesort_registros(Registro *v, size_t n) {
    Registro *aux;
    if (n<2) return 1;
    if (n>SIZE_MAX/sizeof *aux) return 0;
    aux=malloc(n*sizeof *aux); if (!aux) return 0;
    ordenar(v,aux,0,n); free(aux); return 1;
}
