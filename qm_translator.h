#ifndef QMTRANSLATORX_H
#define QMTRANSLATORX_H

#include <string>
#include <vector>

class QmTranslatorX
{
    unsigned char*  FileData;
    int             FileLength;

    // Pointers and offsets into FileData[FileLength] array, or user
    // provided data array
    unsigned char*  messageArray;
    unsigned char*  offsetArray;
    unsigned char*  contextArray;
    unsigned char*  numerusRulesArray;
    unsigned int    messageLength;
    unsigned int    offsetLength;
    unsigned int    contextLength;
    unsigned int    numerusRulesLength;
    std::vector<QmTranslatorX*> subTranslators;

public:
    QmTranslatorX();
    virtual ~QmTranslatorX();

    std::u16string do_translate(const char *context, const char *sourceText,
                                const char *comment = NULL, int n = -1);
    std::string    do_translate8(const char *context, const char *sourceText,
                                 const char *comment = NULL, int n = -1);
    std::u32string do_translate32(const char *context, const char *sourceText,
                                  const char *comment = NULL, int n = -1);

    bool loadFile(const char*filePath, unsigned char *directory = 0);
    bool loadData(unsigned char*data, int FileLength, unsigned char *directory = 0);
    void close();

};

#endif // QMTRANSLATORX_H
