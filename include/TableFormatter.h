#ifndef TABLEFORMATTER_H
#define TABLEFORMATTER_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

// Clase para formatear tablas de texto
class TableFormatter {
private:
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    std::vector<size_t> columnWidths;
    
    void calculateColumnWidths();
    
public:
    TableFormatter(const std::vector<std::string>& headers);
    void addRow(const std::vector<std::string>& row);
    std::string toString() const;
    void clear();
};

#endif // TABLEFORMATTER_H
