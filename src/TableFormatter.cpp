#include "TableFormatter.h"
#include <algorithm>
#include <iostream>

TableFormatter::TableFormatter(const std::vector<std::string>& headers) 
    : headers(headers) {
    columnWidths.resize(headers.size(), 0);
    calculateColumnWidths();
}

void TableFormatter::calculateColumnWidths() {
    // Inicializar con el ancho de los encabezados
    for (size_t i = 0; i < headers.size(); ++i) {
        columnWidths[i] = headers[i].length();
    }

    // Actualizar con el ancho máximo de cada columna en las filas
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size() && i < columnWidths.size(); ++i) {
            columnWidths[i] = std::max(columnWidths[i], row[i].length());
        }
    }

    // Agregar padding adicional a todas las columnas
    for (size_t i = 0; i < columnWidths.size(); ++i) {
        columnWidths[i] += 7;
    }
}

void TableFormatter::addRow(const std::vector<std::string>& row) {
    rows.push_back(row);
    calculateColumnWidths();
}

std::string TableFormatter::toString() const {
    std::ostringstream oss;
    
    // Línea de encabezado
    for (size_t i = 0; i < headers.size(); ++i) {
        if (i > 0) oss << " ";
        oss << std::left << std::setw(columnWidths[i]) << headers[i];
    }
    oss << "\n";
    
    // Línea separadora (guiones)
    size_t totalWidth = 0;
    for (size_t i = 0; i < columnWidths.size(); ++i) {
        totalWidth += columnWidths[i];
        if (i < columnWidths.size() - 1) totalWidth += 1; // espacios entre columnas
    }
    oss << std::string(totalWidth, '-') << "\n";
    
    // Filas de datos
    for (const auto& row : rows) {
        for (size_t i = 0; i < columnWidths.size(); ++i) {
            if (i > 0) oss << " ";
            std::string value = (i < row.size()) ? row[i] : "";
            oss << std::left << std::setw(columnWidths[i]) << value;
        }
        oss << "\n";
    }
    
    // Línea de cierre (igual signo)
    oss << std::string(totalWidth, '=') << "\n";
    
    return oss.str();
}

void TableFormatter::clear() {
    rows.clear();
    columnWidths.resize(headers.size(), 0);
    calculateColumnWidths();
}
