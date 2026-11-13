from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

# Rutas del proyecto
BASE = Path(__file__).resolve().parent
ARCHIVO_CSV = BASE / "resultados" / "datos_crudos.csv"
CARPETA_GRAFICAS = BASE / "graficas"
CARPETA_RESULTADOS = BASE / "resultados"

CARPETA_GRAFICAS.mkdir(exist_ok=True)
CARPETA_RESULTADOS.mkdir(exist_ok=True)

# Comprobar que existe el archivo de mediciones
if not ARCHIVO_CSV.exists():
    raise FileNotFoundError(
        f"No se encontró el archivo: {ARCHIVO_CSV}"
    )

# Leer los datos
df = pd.read_csv(ARCHIVO_CSV)

# Limpiar nombres de columnas y datos
df.columns = df.columns.str.strip()

columnas_necesarias = {
    "prueba", "tipo", "tamano", "patron",
    "optimizacion", "repeticion", "tiempo_ms"
}

faltantes = columnas_necesarias - set(df.columns)
if faltantes:
    raise ValueError(f"Faltan columnas en el CSV: {faltantes}")

df["tiempo_ms"] = pd.to_numeric(df["tiempo_ms"], errors="coerce")
df = df.dropna(subset=["tiempo_ms"])

# Normalizar textos
for columna in ["prueba", "tipo", "tamano", "patron", "optimizacion"]:
    df[columna] = df[columna].astype(str).str.strip()

# Calcular estadísticas
grupos = [
    "prueba", "tipo", "tamano", "patron", "optimizacion"
]

estadisticas = (
    df.groupby(grupos)["tiempo_ms"]
    .agg(
        cantidad="count",
        promedio="mean",
        mediana="median",
        minimo="min",
        maximo="max"
    )
    .reset_index()
)

estadisticas.to_csv(
    CARPETA_RESULTADOS / "estadisticas.csv",
    index=False,
    encoding="utf-8-sig"
)

print("\n========== ESTADÍSTICAS DEL BENCHMARK ==========\n")
print(estadisticas.to_string(index=False, float_format=lambda x: f"{x:.4f}"))
print("\nEstadísticas guardadas en resultados/estadisticas.csv")

# -------------------------------------------------
# GRÁFICA 1: Rendimiento de CPU por optimización
# -------------------------------------------------
cpu = df[df["prueba"].str.upper() == "CPU"].copy()

if not cpu.empty:
    resumen_cpu = (
        cpu.groupby(["tipo", "tamano", "optimizacion"])["tiempo_ms"]
        .mean()
        .reset_index()
    )

    resumen_cpu["configuracion"] = (
        resumen_cpu["tipo"] + " - N=" + resumen_cpu["tamano"]
    )

    tabla_cpu = resumen_cpu.pivot(
        index="configuracion",
        columns="optimizacion",
        values="tiempo_ms"
    )

    ax = tabla_cpu.plot(kind="bar", figsize=(11, 6))
    ax.set_title("Tiempo promedio de ejecución de CPU")
    ax.set_xlabel("Configuración")
    ax.set_ylabel("Tiempo promedio (ms)")
    ax.legend(title="Optimización")
    ax.tick_params(axis="x", rotation=20)
    ax.grid(axis="y", alpha=0.3)
    plt.tight_layout()
    plt.savefig(CARPETA_GRAFICAS / "grafica_1_cpu.png", dpi=300)
    plt.close()

# -------------------------------------------------
# GRÁFICA 2: Acceso a memoria secuencial y aleatorio
# -------------------------------------------------
memoria = df[df["prueba"].str.upper() == "MEMORIA"].copy()

if not memoria.empty:
    resumen_memoria = (
        memoria.groupby(
            ["tamano", "patron", "optimizacion"]
        )["tiempo_ms"]
        .mean()
        .reset_index()
    )

    resumen_memoria["configuracion"] = (
        resumen_memoria["tamano"] + " - " + resumen_memoria["patron"]
    )

    tabla_memoria = resumen_memoria.pivot(
        index="configuracion",
        columns="optimizacion",
        values="tiempo_ms"
    )

    ax = tabla_memoria.plot(kind="bar", figsize=(12, 6))
    ax.set_title("Tiempo promedio de acceso a memoria")
    ax.set_xlabel("Tamaño y patrón de acceso")
    ax.set_ylabel("Tiempo promedio (ms)")
    ax.set_yscale("log")
    ax.legend(title="Optimización")
    ax.tick_params(axis="x", rotation=25)
    ax.grid(axis="y", alpha=0.3)
    plt.tight_layout()
    plt.savefig(CARPETA_GRAFICAS / "grafica_2_memoria.png", dpi=300)
    plt.close()

# -------------------------------------------------
# GRÁFICA 3: Aceleración de CPU respecto a O0
# -------------------------------------------------
if not cpu.empty:
    resumen = (
        cpu.groupby(["tipo", "tamano", "optimizacion"])["tiempo_ms"]
        .mean()
        .reset_index()
    )

    tabla = resumen.pivot(
        index=["tipo", "tamano"],
        columns="optimizacion",
        values="tiempo_ms"
    )

    if "O0" in tabla.columns:
        aceleracion = tabla.copy()

        for optimizacion in ["O2", "O3"]:
            if optimizacion in aceleracion.columns:
                aceleracion[optimizacion] = (
                    aceleracion["O0"] / aceleracion[optimizacion]
                )

        aceleracion = aceleracion.drop(columns=["O0"])
        aceleracion.index = [
            f"{tipo} - N={tamano}"
            for tipo, tamano in aceleracion.index
        ]

        ax = aceleracion.plot(kind="bar", figsize=(11, 6))
        ax.set_title("Aceleración respecto a la compilación O0")
        ax.set_xlabel("Configuración de CPU")
        ax.set_ylabel("Factor de aceleración (veces)")
        ax.tick_params(axis="x", rotation=20)
        ax.grid(axis="y", alpha=0.3)
        plt.tight_layout()
        plt.savefig(
            CARPETA_GRAFICAS / "grafica_3_aceleracion.png",
            dpi=300
        )
        plt.close()

print("\n========== PROCESO FINALIZADO ==========")
print("Archivos generados:")
print("- resultados/estadisticas.csv")
print("- graficas/grafica_1_cpu.png")
print("- graficas/grafica_2_memoria.png")
print("- graficas/grafica_3_aceleracion.png")