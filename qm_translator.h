#ifndef QMTRANSLATORX_H
#define QMTRANSLATORX_H

#include <string>

class QmTranslatorX
{
    unsigned char *FileData = NULL;
    int            FileLength  = 0;
    // Pointers and offsets into unmapPointer[unmapLength] array, or user
    // provided data array
    unsigned char *messageArray = NULL;
    unsigned char *offsetArray = NULL;
    unsigned char *contextArray = NULL;
    unsigned char *numerusRulesArray = NULL;
    unsigned int messageLength = 0;
    unsigned int offsetLength = 0;
    unsigned int contextLength = 0;
    unsigned int numerusRulesLength = 0;

public:
    QmTranslatorX();
    virtual ~QmTranslatorX();
    std::wstring do_translate(const char *context, const char *sourceText,
                              const char *comment = NULL, int n = -1);

    bool loadFile(const char*filePath);
    bool loadData(unsigned char*data, int FileLength);
    void close();

};

#endif // QMTRANSLATORX_H
