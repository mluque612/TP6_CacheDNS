# 🧩 TP6 — Sistema de Caché DNS con Tabla Hash  
**Materia:** Informatica 2  
**Alumno:** Luque-Rivata, Mateo  
**Año:** 2025  

---

## 📘 Descripción General

Este trabajo práctico implementa un **sistema de caché DNS** utilizando una **tabla hash con manejo de colisiones mediante listas enlazadas (encadenamiento)**.

El objetivo es aplicar los conceptos de:

- **Estructuras dinámicas** (`struct`, `malloc`, `free`)
- **Listas simplemente enlazadas**
- **Estructuras anidadas**
- **Gestión de memoria**
- **Uso de funciones hash**

El sistema permite simular un **caché DNS simplificado**, donde se almacenan entradas con dominio, IP, TTL (tiempo de vida) y estadísticas de consulta.

---

## 🧠 Fundamento Teórico

### ¿Qué es una Tabla Hash?
Una **tabla hash** es una estructura de datos que asocia una clave con un valor, permitiendo inserciones y búsquedas promedio en **O(1)**.  
Para calcular la posición de almacenamiento, se aplica una **función hash** sobre la clave (en este caso, el nombre de dominio).

### Encadenamiento de colisiones
Cuando dos claves producen el mismo índice, se usa **encadenamiento**:  
cada posición del arreglo (bucket) almacena una **lista enlazada** con todas las entradas que comparten ese hash.

### Aplicación al DNS
Un sistema de nombres de dominio (DNS) traduce nombres (ej. `www.google.com`) en direcciones IP.  
El **caché DNS** guarda resultados para evitar consultas repetidas.  
Cada registro tiene un **TTL (Time To Live)** que indica cuándo expira.

---

## ⚙️ Estructura del Proyecto

### 🧩 Estructura de carpetas sugerida

```text
📦 TP6_CacheDNS/
 ├── tp6_dns_cache.c     ← todo el código en este archivo
 ├── informe-tp6.tex
 ├── README.md
 └── LICENSE

---

| Función            | Descripción                                           | Complejidad   |
|--------------------|-------------------------------------------------------|---------------|
| `hash_djb2()`      | Calcula el hash del dominio (función DJB2)            | O(n)          |
| `insertar()`       | Inserta o actualiza una entrada en la tabla           | O(1) promedio |
| `buscar()`         | Busca un dominio y devuelve puntero al nodo           | O(1) promedio |
| `eliminar()`       | Elimina una entrada por dominio                       | O(1) promedio |
| `limpiar_expirados()` | Elimina entradas vencidas (TTL)                    | O(n)          |
| `estadisticas()`   | Muestra colisiones, factor de carga y totales         | O(n)          |
| `mostrar_todos()`  | Recorre toda la tabla hash                            | O(n)          |
| `generar_datos()`  | Crea datos DNS aleatorios (IPs, TTL, hits)            | O(k)          |

---

🧩 Menú del Programa

=== Sistema de Cache DNS ===
1. Cachear nueva entrada (insertar/actualizar)
2. Buscar dominio
3. Actualizar (pedir datos y sobrescribir)
4. Eliminar entrada
5. Limpiar expirados por TTL
6. Mostrar bucket
7. Mostrar todos los dominios
8. Mostrar estadisticas
9. Generar datos aleatorios
0. Salir

---

## 🧰 Compilación y Ejecución

```bash
gcc -std=c11 -Wall -Wextra -O2 -o tp6 tp6_dns_cache.c
./tp6

---

=== Sistema de Cache DNS ===
> 9
Cantidad a generar: 10
Se generaron 10 entradas de prueba.

> 8
Entradas totales: 10
Buckets: 50 | Vacíos: 42 | Buckets con colisión: 1
Factor de carga: 0.20
Longitud máxima de un bucket: 2

## 📜 Licencia

Este proyecto está bajo una **Licencia de Uso Educativo y Académico**.

Podés consultar el texto completo en el archivo [LICENSE].

Autor: **Mateo Luque-Rivata**  
Instituto Univversitario Aeronautico — 2025

