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
│
├── src/
│   ├── main.c              # Función principal, menú de usuario
│   ├── hash.c              # Implementación de la tabla hash (insertar, buscar, eliminar)
│   ├── hash.h              # Definición de estructuras y prototipos
│   ├── dns.c               # Manejo de estructuras DNS (crear, mostrar, generar)
│   ├── dns.h               # Definiciones de Registro, Metadatos, Estadísticas
│   ├── utiles.c            # Funciones auxiliares (leer_linea, IP aleatoria, tiempo)
│   ├── utiles.h            # Headers de utilidades
│
├── docs/
│   ├── informe-tp6.tex     # Informe LaTeX
│   ├── informe-tp6.pdf     # Informe compilado
│
├── README.md               # Documentación del proyecto
├── Makefile (opcional)     # Compilación automática
└── tp6.exe / tp6           # Ejecutable final



