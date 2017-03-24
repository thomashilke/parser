#include <algorithm>

#include "stream_array.hpp"

StreamArray::EndOfRow StreamArray::endr;
StreamArray::BlankCell StreamArray::blank;

StreamArray::AlignCol StreamArray::left = {true};
StreamArray::AlignCol StreamArray::right = {false};

template<>
StreamArray& StreamArray::operator<< <StreamArray::EndOfRow>(const StreamArray::EndOfRow&)
{ lineBreaks.push_back(cellValues.size()); return *this; }

template<>
StreamArray& StreamArray::operator<< <StreamArray::BlankCell>(const StreamArray::BlankCell&)
{ cellValues.push_back("");  return *this;}

void StreamArray::flush()
{
  if(lineBreaks.back() != cellValues.size())
    operator<< <EndOfRow>(endr);

  std::vector<unsigned int> rowSizes(lineBreaks.size(), 0);
  std::adjacent_difference(lineBreaks.begin(), lineBreaks.end(), rowSizes.begin());

  const std::size_t columns(*std::max_element(rowSizes.begin(), rowSizes.end()));

  std::vector<std::size_t> columnWidths(columns, 0);
  for(unsigned int col(0); col < columns; ++col)
    {
      unsigned int cellId(0);
      for(unsigned int row(0); row < rowSizes.size(); ++row)
        {
          if(rowSizes[row] > col)
            columnWidths[col] = std::max(columnWidths[col],
                                         cellValues[cellId + col].size());
          cellId += rowSizes[row];
        }
    }

  alignment.resize(columns, false);

  unsigned int cellId(0);
  for(unsigned int row(0); row < rowSizes.size(); ++row)
    {
      for(std::size_t col(0); col < columns; ++col)
        {
          if(rowSizes[row] > col)
            {
              flux << std::setw(static_cast<int>(columnWidths[col])) 
                   << (alignment[col] ? std::left : std::right)
                   << cellValues[cellId];
              ++cellId;
            }
          else
            flux << std::setw(static_cast<int>(columnWidths[col])) << "";

          if(col != columns - 1)
            flux << separator;
        }
      flux << std::endl;
    }
  
  cellValues.clear();
  lineBreaks.clear();
}
