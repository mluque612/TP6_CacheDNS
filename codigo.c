/*
 * TP6 — Sistema de Caché DNS con Tabla Hash (encadenamiento)
 * Compilar: gcc -Wall -Wextra -O2 -o tp6 tp6_dns_cache.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* ---------- Configuración ---------- */
#define TAM_TABLA 50

/* ---------- Modelado de IP v4 ---------- */
typedef struct {
    unsigned char o1, o2, o3, o4;  /* 0..255 */
} IPv4;

/* ---------- Estructuras anidadas ---------- */
typedef struct {
    char dominio[100];   /* p.ej. "www.example.com" */
    char tipo[10];       /* "A", "AAAA", "CNAME", "MX" */
    IPv4 ip_resuelta;    /* válido si tipo == "A" */
    char ip_v6[40];      /* válido si tipo == "AAAA" (opcional) */
} Registro;

typedef struct {
    int  ttl_segundos;       /* Time To Live */
    time_t tiempo_cache;     /* timestamp de cacheo (epoch) */
    int  hits;               /* consultas acumuladas */
    char servidor_origen[50];/* p.ej. "8.8.8.8" */
} Metadatos;

typedef struct {
    int  tiempo_resolucion_ms;/* simulación/registro */
    int  prioridad;           /* para MX */
    char alias[100];          /* para CNAME */
} Estadisticas;

typedef struct {
    Registro    registro;
    Metadatos   meta;
    Estadisticas stats;
} EntradaDNS;

/* ---------- Nodo de lista (encadenamiento) ---------- */
typedef struct Nodo {
    EntradaDNS entrada;
    struct Nodo *siguiente;
} Nodo;

/* ---------- Tabla hash ---------- */
typedef struct {
    Nodo *buckets[TAM_TABLA];
} TablaHash;

/* ---------- Utilidades ---------- */
static void normalizar_minusculas(char *s) {
    for (; *s; ++s) *s = (char)tolower((unsigned char)*s);
}

static void imprimir_ipv4(IPv4 ip) {
    printf("%u.%u.%u.%u", ip.o1, ip.o2, ip.o3, ip.o4);
}

static int parse_ipv4(const char *txt, IPv4 *out) {
    int a,b,c,d;
    if (sscanf(txt, "%d.%d.%d.%d", &a,&b,&c,&d) != 4) return 0;
    if (a<0||a>255||b<0||b>255||c<0||c>255||d<0||d>255) return 0;
    out->o1=(unsigned char)a; out->o2=(unsigned char)b;
    out->o3=(unsigned char)c; out->o4=(unsigned char)d;
    return 1;
}

/* djb2 hash */
static unsigned long hash_djb2(const char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        h = ((h << 5) + h) + c; /* h*33 + c */
    }
    return h;
}

static int indice_hash(const char *dominio) {
    char tmp[128];
    strncpy(tmp, dominio, sizeof(tmp)-1);
    tmp[sizeof(tmp)-1] = '\0';
    normalizar_minusculas(tmp);
    return (int)(hash_djb2(tmp) % TAM_TABLA);
}

/* ¿Expiró según TTL? */
static int expiro(const Metadatos *m) {
    if (m->ttl_segundos <= 0) return 0; /* TTL<=0 => no expira */
    time_t ahora = time(NULL);
    double dt = difftime(ahora, m->tiempo_cache);
    return dt > (double)m->ttl_segundos;
}

/* ---------- API de tabla hash ---------- */
static void init_tabla(TablaHash *t) {
    for (int i=0;i<TAM_TABLA;i++) t->buckets[i] = NULL;
}

static void liberar_tabla(TablaHash *t) {
    for (int i=0;i<TAM_TABLA;i++) {
        Nodo *cur = t->buckets[i];
        while (cur) {
            Nodo *nx = cur->siguiente;
            free(cur);
            cur = nx;
        }
        t->buckets[i] = NULL;
    }
}

/* Inserta o actualiza (si ya existe el dominio) */
static void insertar(TablaHash *t, const EntradaDNS *e) {
    int idx = indice_hash(e->registro.dominio);
    Nodo *cur = t->buckets[idx];

    while (cur) {
        if (strcmp(cur->entrada.registro.dominio, e->registro.dominio)==0) {
            /* Actualiza */
            cur->entrada = *e;
            return;
        }
        cur = cur->siguiente;
    }
    /* No existe: insertar al inicio */
    Nodo *nuevo = (Nodo*)malloc(sizeof(Nodo));
    if (!nuevo) { fprintf(stderr, "Error: sin memoria.\n"); return; }
    nuevo->entrada = *e;
    nuevo->siguiente = t->buckets[idx];
    t->buckets[idx] = nuevo;
}

/* Devuelve puntero a la entrada o NULL. Si foundBucket y prev no son NULL, los completa */
static EntradaDNS* buscar(TablaHash *t, const char *dominio, int *outIdx, Nodo **outPrev, Nodo **outNode) {
    int idx = indice_hash(dominio);
    if (outIdx) *outIdx = idx;
    Nodo *prev = NULL;
    for (Nodo *cur = t->buckets[idx]; cur; prev = cur, cur = cur->siguiente) {
        if (strcmp(cur->entrada.registro.dominio, dominio)==0) {
            if (outPrev) *outPrev = prev;
            if (outNode) *outNode = cur;
            return &cur->entrada;
        }
    }
    return NULL;
}

static int eliminar(TablaHash *t, const char *dominio) {
    int idx; Nodo *prev=NULL, *node=NULL;
    if (!buscar(t, dominio, &idx, &prev, &node)) return 0;
    if (!prev) t->buckets[idx] = node->siguiente;
    else prev->siguiente = node->siguiente;
    free(node);
    return 1;
}

static int limpiar_expirados(TablaHash *t) {
    int eliminadas = 0;
    for (int i=0;i<TAM_TABLA;i++) {
        Nodo *prev = NULL, *cur = t->buckets[i];
        while (cur) {
            if (expiro(&cur->entrada.meta)) {
                Nodo *bor = cur;
                if (!prev) t->buckets[i] = cur->siguiente;
                else prev->siguiente = cur->siguiente;
                cur = cur->siguiente;
                free(bor);
                eliminadas++;
            } else {
                prev = cur;
                cur = cur->siguiente;
            }
        }
    }
    return eliminadas;
}

/* ---------- Reporting ---------- */
static void imprimir_entrada(const EntradaDNS *e) {
    printf("Dominio: %s\n", e->registro.dominio);
    printf("Tipo: %s\n", e->registro.tipo);
    if (strcmp(e->registro.tipo,"A")==0) {
        printf("IP: "); imprimir_ipv4(e->registro.ip_resuelta); printf("\n");
    } else if (strcmp(e->registro.tipo,"AAAA")==0) {
        printf("IPv6: %s\n", e->registro.ip_v6);
    }
    if (strcmp(e->registro.tipo,"CNAME")==0 && e->stats.alias[0])
        printf("Alias(CNAME): %s\n", e->stats.alias);
    if (strcmp(e->registro.tipo,"MX")==0)
        printf("Prioridad(MX): %d\n", e->stats.prioridad);

    struct tm *tmc = localtime(&e->meta.tiempo_cache);
    char buf[64]; strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S", tmc);

    printf("TTL: %d s | Cacheado: %s | Hits: %d | Origen: %s\n",
           e->meta.ttl_segundos, buf, e->meta.hits, e->meta.servidor_origen);
    printf("t_resolucion: %d ms\n", e->stats.tiempo_resolucion_ms);
}

static void mostrar_bucket(const TablaHash *t, int idx) {
    if (idx < 0 || idx >= TAM_TABLA) { printf("Indice fuera de rango.\n"); return; }
    printf("=== Bucket %d ===\n", idx);
    int n=0;
    for (Nodo *cur = t->buckets[idx]; cur; cur = cur->siguiente, ++n) {
        printf("- [%d]\n", n);
        imprimir_entrada(&cur->entrada);
    }
    if (n==0) printf("(vacío)\n");
}

static void mostrar_todos(const TablaHash *t) {
    for (int i=0;i<TAM_TABLA;i++) {
        int n=0;
        for (Nodo *cur = t->buckets[i]; cur; cur = cur->siguiente) n++;
        if (n>0) {
            printf("\n--- Bucket %d (len=%d) ---\n", i, n);
            mostrar_bucket(t, i);
        }
    }
}

static void estadisticas(const TablaHash *t) {
    int total=0, vacios=0, colisiones=0, maxlen=0;
    for (int i=0;i<TAM_TABLA;i++) {
        int len=0;
        for (Nodo *cur = t->buckets[i]; cur; cur = cur->siguiente) len++;
        total += len;
        if (len==0) vacios++;
        if (len>=2) colisiones++;
        if (len>maxlen) maxlen=len;
    }
    printf("Entradas totales: %d\n", total);
    printf("Buckets: %d | Vacios: %d | Buckets con colision(>=2): %d\n", TAM_TABLA, vacios, colisiones);
    printf("Factor de carga: %.3f\n", (double)total / (double)TAM_TABLA);
    printf("Longitud maxima de un bucket: %d\n", maxlen);
}

/* ---------- I/O usuario ---------- */
static void leer_linea(char *buf, size_t n) {
    if (!fgets(buf, (int)n, stdin)) {
        buf[0] = '\0';
        return;
    }
    size_t L = strlen(buf);
    while (L > 0 && (buf[L - 1] == '\n' || buf[L - 1] == '\r')) {
        buf[--L] = '\0';
    }
}


static void pedir_entrada(EntradaDNS *e) {
    char tmp[128];

    printf("Dominio: ");
    leer_linea(e->registro.dominio, sizeof(e->registro.dominio));
    if (e->registro.dominio[0]=='\0') strcpy(e->registro.dominio, "example.com");
    normalizar_minusculas(e->registro.dominio);

    printf("Tipo (A/AAAA/CNAME/MX): ");
    leer_linea(e->registro.tipo, sizeof(e->registro.tipo));
    if (e->registro.tipo[0]=='\0') strcpy(e->registro.tipo, "A");
    for (char *p=e->registro.tipo; *p; ++p) *p=(char)toupper((unsigned char)*p);

    if (strcmp(e->registro.tipo,"A")==0) {
        printf("IP v4 (a.b.c.d): ");
        leer_linea(tmp, sizeof(tmp));
        if (!parse_ipv4(tmp, &e->registro.ip_resuelta)) {
            parse_ipv4("93.184.216.34", &e->registro.ip_resuelta); /* default */
        }
    } else if (strcmp(e->registro.tipo,"AAAA")==0) {
        printf("IPv6: ");
        leer_linea(e->registro.ip_v6, sizeof(e->registro.ip_v6));
        if (e->registro.ip_v6[0]=='\0') strcpy(e->registro.ip_v6, "2001:db8::1");
    } else if (strcmp(e->registro.tipo,"CNAME")==0) {
        printf("Alias CNAME de: ");
        leer_linea(e->stats.alias, sizeof(e->stats.alias));
        if (e->stats.alias[0]=='\0') strcpy(e->stats.alias, "target.example.com");
    } else if (strcmp(e->registro.tipo,"MX")==0) {
        printf("Prioridad MX (entero): ");
        leer_linea(tmp, sizeof(tmp));
        e->stats.prioridad = atoi(tmp);
        if (e->stats.prioridad<=0) e->stats.prioridad = 10;
    }

    printf("TTL (segundos, 0 = no expira): ");
    leer_linea(tmp, sizeof(tmp));
    e->meta.ttl_segundos = atoi(tmp);
    if (e->meta.ttl_segundos<0) e->meta.ttl_segundos = 0;

    printf("Servidor origen (p.ej. 8.8.8.8): ");
    leer_linea(e->meta.servidor_origen, sizeof(e->meta.servidor_origen));
    if (e->meta.servidor_origen[0]=='\0') strcpy(e->meta.servidor_origen, "8.8.8.8");

    printf("Tiempo de resolución (ms): ");
    leer_linea(tmp, sizeof(tmp));
    e->stats.tiempo_resolucion_ms = atoi(tmp);
    if (e->stats.tiempo_resolucion_ms<=0) e->stats.tiempo_resolucion_ms = 25;

    /* set defaults */
    e->meta.tiempo_cache = time(NULL);
    e->meta.hits = 0;
    if (strcmp(e->registro.tipo,"CNAME")!=0) e->stats.alias[0]='\0';
}

/* ---------- Datos aleatorios para probar ---------- */
static int rnd(int a, int b){ return a + rand()%(b-a+1); }
static void rnd_ipv4(IPv4 *ip){ ip->o1=rnd(1,223); ip->o2=rnd(0,255); ip->o3=rnd(0,255); ip->o4=rnd(0,255); }

static void generar_datos(TablaHash *t, int cantidad) {
    const char *doms[] = {"google.com","facebook.com","youtube.com","amazon.com","wikipedia.org",
                          "api.servicio.io","cdn.example.com","mail.empresa.com","vpn.empresa.com","blog.example.com"};
    int nd = (int)(sizeof(doms)/sizeof(doms[0]));
    for (int i=0;i<cantidad;i++) {
        EntradaDNS e = {0};
        snprintf(e.registro.dominio, sizeof(e.registro.dominio), "%s", doms[rnd(0,nd-1)]);
        strcpy(e.registro.tipo, "A");
        rnd_ipv4(&e.registro.ip_resuelta);
        e.meta.ttl_segundos = (int[]){300,600,1800,3600,86400}[rnd(0,4)];
        e.meta.tiempo_cache = time(NULL) - rnd(0, e.meta.ttl_segundos/2);
        e.meta.hits = rnd(0,50);
        strcpy(e.meta.servidor_origen, (const char*[]){"8.8.8.8","1.1.1.1","9.9.9.9"}[rnd(0,2)]);
        e.stats.tiempo_resolucion_ms = rnd(8,120);
        insertar(t, &e);
    }
    printf("Se generaron %d entradas de prueba.\n", cantidad);
}

/* ---------- Menú ---------- */
static void menu(void) {
    printf("\n=== Sistema de Cache DNS ===\n");
    printf("1. Cachear nueva entrada (insertar/actualizar)\n");
    printf("2. Buscar dominio\n");
    printf("3. Actualizar (pedir datos y sobrescribir)\n");
    printf("4. Eliminar entrada\n");
    printf("5. Limpiar expirados por TTL\n");
    printf("6. Mostrar bucket\n");
    printf("7. Mostrar todos los dominios\n");
    printf("8. Mostrar estadisticas\n");
    printf("9. Generar datos aleatorios\n");
    printf("0. Salir\n> ");
}

int main(void) {
    TablaHash tabla;
    init_tabla(&tabla);
    srand((unsigned)time(NULL));

    char buf[128];
    for (;;) {
        menu();
        leer_linea(buf, sizeof(buf));
        int op = atoi(buf);

        if (op == 0) break;

        if (op == 1) {
            EntradaDNS e = {0};
            pedir_entrada(&e);
            insertar(&tabla, &e);
            printf("Entrada cacheada/actualizada correctamente.\n");
        }
        else if (op == 2) {
            char dominio[100];
            printf("Dominio a buscar: ");
            leer_linea(dominio, sizeof(dominio));
            normalizar_minusculas(dominio);
            EntradaDNS *en = buscar(&tabla, dominio, NULL, NULL, NULL);
            if (!en) { printf("No encontrado.\n"); }
            else {
                en->meta.hits++;
                imprimir_entrada(en);
                if (expiro(&en->meta)) printf("⚠ Entrada EXPIRADA (TTL superado)\n");
            }
        }
        else if (op == 3) {
            char dominio[100];
            printf("Dominio a actualizar: ");
            leer_linea(dominio, sizeof(dominio));
            normalizar_minusculas(dominio);
            EntradaDNS *en = buscar(&tabla, dominio, NULL, NULL, NULL);
            if (!en) { printf("No existe. Use opcion 1 para insertarlo.\n"); }
            else {
                EntradaDNS e2 = *en; /* precargar */
                pedir_entrada(&e2);
                /* forzar dominio original si desea mantener la clave */
                if (e2.registro.dominio[0]=='\0') strncpy(e2.registro.dominio, dominio, sizeof(e2.registro.dominio)-1);
                insertar(&tabla, &e2);
                printf("Actualizado.\n");
            }
        }
        else if (op == 4) {
            char dominio[100];
            printf("Dominio a eliminar: ");
            leer_linea(dominio, sizeof(dominio));
            normalizar_minusculas(dominio);
            if (eliminar(&tabla, dominio)) printf("Eliminado.\n");
            else printf("No existe.\n");
        }
        else if (op == 5) {
            int n = limpiar_expirados(&tabla);
            printf("Se eliminaron %d entradas expiradas.\n", n);
        }
        else if (op == 6) {
            printf("Indice de bucket [0..%d]: ", TAM_TABLA-1);
            leer_linea(buf, sizeof(buf));
            int idx = atoi(buf);
            mostrar_bucket(&tabla, idx);
        }
        else if (op == 7) {
            mostrar_todos(&tabla);
        }
        else if (op == 8) {
            estadisticas(&tabla);
        }
        else if (op == 9) {
            printf("Cantidad a generar: ");
            leer_linea(buf, sizeof(buf));
            int n = atoi(buf);
            if (n<=0) n = 10;
            generar_datos(&tabla, n);
        }
        else {
            printf("Opcion invalida.\n");
        }
    }

    liberar_tabla(&tabla);
    printf("Hasta luego.\n");
    return 0;
}
