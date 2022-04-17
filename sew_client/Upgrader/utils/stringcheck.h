#ifndef STRINGCHECK_H
#define STRINGCHECK_H

#include <QVector>

class StringCheck
{
public:
    StringCheck();
    static QVector<int> KMP_Check(const char *text, const char *pattern, bool *hasFound);

private:
    static void  getPrefixTable(const char *pattern, int patternLen, int *prefixTable);
};

#endif // STRINGCHECK_H
