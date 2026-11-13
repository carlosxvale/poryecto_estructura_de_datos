#include <iostream>
#include <vector>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>
#include <numeric>
#include <algorithm>
#include <random>
#include <filesystem>
#include <iomanip>
#include <stdexcept>

using namespace std;
using namespace std::chrono;

#ifndef OPT_LABEL
#define OPT_LABEL UNKNOWN
#endif

#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)

const string OPTIMIZACION = STRINGIFY(OPT_LABEL);
const int CALENTAMIENTOS = 2;
const int REPETICIONES = 10;

volatile long double resultadoGlobal = 0;

ofstream archivoCSV;

double milisegundos(steady_clock::time_point inicio,
                    steady_clock::time_point fin) {
    return duration<double, milli>(fin - inicio).count();
}

void prepararCSV(const string& ruta) {
    filesystem::path rutaArchivo(ruta);

    if (!rutaArchivo.parent_path().empty()) {
        filesystem::create_directories(rutaArchivo.parent_path());
    }

    bool existe = filesystem::exists(rutaArchivo) &&
                  filesystem::file_size(rutaArchivo) > 0;

    archivoCSV.open(ruta, ios::app);

    if (!archivoCSV.is_open()) {
        throw runtime_error("No se pudo abrir el archivo CSV.");
    }

    if (!existe) {
        archivoCSV
            << "prueba,tipo,tamano,patron,optimizacion,"
            << "repeticion,tiempo_ms\n";
    }
}

void guardarResultado(const string& prueba,
                      const string& tipo,
                      const string& tamano,
                      const string& patron,
                      int repeticion,
                      double tiempo) {

    archivoCSV << prueba << ","
               << tipo << ","
               << tamano << ","
               << patron << ","
               << OPTIMIZACION << ","
               << repeticion << ","
               << fixed << setprecision(6)
               << tiempo << "\n";

    archivoCSV.flush();
}

// --------------------------------------------------
// PRUEBA A: MULTIPLICACION DE MATRICES
// --------------------------------------------------

template <typename T>
double multiplicarMatrices(int n) {

    vector<T> A(n * n);
    vector<T> B(n * n);
    vector<T> C(n * n, 0);

    for (int i = 0; i < n * n; i++) {
        A[i] = static_cast<T>((i % 10) + 1);
        B[i] = static_cast<T>((i % 7) + 1);
    }

    auto inicio = steady_clock::now();

    for (int i = 0; i < n; i++) {
        for (int k = 0; k < n; k++) {

            T valor = A[i * n + k];

            for (int j = 0; j < n; j++) {
                C[i * n + j] += valor * B[k * n + j];
            }
        }
    }

    auto fin = steady_clock::now();

    long double suma = 0;

    for (const auto& valor : C) {
        suma += valor;
    }

    resultadoGlobal = suma;

    return milisegundos(inicio, fin);
}

template <typename T>
void ejecutarPruebaCPU(const string& tipo) {

    const int tamanos[] = {256, 768};

    for (int n : tamanos) {

        cout << "\nCPU: " << tipo
             << " | Matrices " << n << "x" << n << endl;

        for (int i = 0; i < CALENTAMIENTOS; i++) {
            multiplicarMatrices<T>(n);
        }

        for (int i = 1; i <= REPETICIONES; i++) {

            double tiempo = multiplicarMatrices<T>(n);

            guardarResultado(
                "CPU", tipo, to_string(n),
                "matrices", i, tiempo
            );

            cout << "Repeticion " << i
                 << ": " << tiempo << " ms" << endl;
        }
    }
}

// --------------------------------------------------
// PRUEBA B: MEMORIA
// --------------------------------------------------

void ejecutarPruebaMemoria() {

    const size_t tamanos[] = {
        32 * 1024,
        512 * 1024,
        64 * 1024 * 1024
    };

    mt19937 generador(12345);

    for (size_t bytes : tamanos) {

        size_t cantidad = bytes / sizeof(uint64_t);

        vector<uint64_t> datos(cantidad);
        vector<size_t> indices(cantidad);

        iota(indices.begin(), indices.end(), 0);

        for (size_t i = 0; i < cantidad; i++) {
            datos[i] = i + 1;
        }

        shuffle(indices.begin(), indices.end(), generador);

        string etiqueta = to_string(bytes / 1024) + "KB";

        cout << "\nMEMORIA: " << etiqueta << endl;

        for (int patron = 0; patron < 2; patron++) {

            string nombrePatron =
                (patron == 0) ? "secuencial" : "aleatorio";

            auto medir = [&]() {

                uint64_t suma = 0;

                auto inicio = steady_clock::now();

                if (patron == 0) {

                    for (size_t i = 0; i < cantidad; i++) {
                        suma += datos[i];
                    }

                } else {

                    for (size_t i = 0; i < cantidad; i++) {
                        suma += datos[indices[i]];
                    }
                }

                auto fin = steady_clock::now();

                resultadoGlobal = suma;

                return milisegundos(inicio, fin);
            };

            for (int i = 0; i < CALENTAMIENTOS; i++) {
                medir();
            }

            for (int i = 1; i <= REPETICIONES; i++) {

                double tiempo = medir();

                guardarResultado(
                    "MEMORIA", "uint64_t",
                    etiqueta, nombrePatron, i, tiempo
                );

                cout << nombrePatron
                     << " | Repeticion " << i
                     << ": " << tiempo << " ms" << endl;
            }
        }
    }
}

// --------------------------------------------------
// PROGRAMA PRINCIPAL
// --------------------------------------------------

int main(int argc, char* argv[]) {

    string rutaCSV = "resultados/datos_crudos.csv";

    if (argc >= 2) {
        rutaCSV = argv[1];
    }

    try {

        prepararCSV(rutaCSV);

        cout << "========================================\n";
        cout << " BENCHMARK DE ARQUITECTURA DE COMPUTADORES\n";
        cout << "========================================\n";

        cout << "Optimizacion: " << OPTIMIZACION << endl;
        cout << "Calentamientos: " << CALENTAMIENTOS << endl;
        cout << "Repeticiones: " << REPETICIONES << endl;

        cout << "\n========== PRUEBA A: CPU ==========\n";

        ejecutarPruebaCPU<int64_t>("int64_t");
        ejecutarPruebaCPU<double>("double");

        cout << "\n========== PRUEBA B: MEMORIA ==========\n";

        ejecutarPruebaMemoria();

        archivoCSV.close();

        cout << "\n========================================\n";
        cout << "BENCHMARK FINALIZADO\n";
        cout << "Datos guardados en: " << rutaCSV << endl;
        cout << "========================================\n";

    } catch (const exception& error) {

        cerr << "ERROR: " << error.what() << endl;
        return 1;
    }

    return 0;
}