#include "stringcheck.h"

StringCheck::StringCheck()
{

}

/* 获取前缀表(next数组) */
void StringCheck::getPrefixTable(const char *pattern, int patternLen, int *prefixTable)
{
    prefixTable[0] = 0;
    int len = 0;  //len表示当前已经达到的 最长公共前后缀 的长度
    int i = 1;    //prefixTable[0]已经 == 0, 故从prefixTable[1]开始
    while (i < patternLen) {
        if (pattern[i] == pattern[len]) {
            len++;
            prefixTable[i] = len;
            i++;
        }
        else {
            if (len > 0) {
                //len = 当前len位置前面那个子串的最长公共前后缀长度
                len = prefixTable[len - 1]; //精髓
            }
            else {
                prefixTable[i] = 0; //此时len = 0
                i++;
            }
        }
    }

    //把前缀表(next数组)整体向后挪一格
    int prefixLen = patternLen;
    for (int i = prefixLen - 1; i > 0; i--) {
        prefixTable[i] = prefixTable[i - 1];
    }
    prefixTable[0] = -1;
}

/* KMP查找算法
参数:
text: 主串
pattern: 模式串
hasFound: 出参, 表示是否找到过pattern

返回值:
模式串在主串中的下标集合


T: a b a a c a b a b c a c
P: a b a b c
 */

QVector<int> StringCheck::KMP_Check(const char *text, const char *pattern, bool *hasFound)
{
    QVector<int> posRet;
    *hasFound = false;

    int patternLen = strlen(pattern);
    int textLen = strlen(text);
    if (patternLen > textLen) {
        return posRet;
    }

    //计算前缀表
    int prefix[patternLen];
    getPrefixTable(pattern, patternLen, prefix);

    //开始查找
    //text[i]
    //pattern[j]
    int i, j;
    i = j = 0;

    while (i < textLen) {

        //检测是否在text中找到了一个pattern
        if (j == patternLen - 1 && text[i] == pattern[j]) {

            // cout << "found at " << i - j;
            posRet.push_back(i - j); //保存本次找到的位置
            j = prefix[j];
            *hasFound = true;
        }
        if (text[i] == pattern[j]) {
            i++;
            j++;
        } else {
            j = prefix[j];
            if (j == -1) {
                i++;
                j++;
            }
        }
    }

    return posRet;
}
