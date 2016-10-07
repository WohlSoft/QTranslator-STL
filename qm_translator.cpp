#include <stdio.h>
#include <string>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <vector>
#include <assert.h>

#include "qm_translator.h"
#include "ConvertUTF.h"

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef signed char qint8;         /* 8 bit signed */
typedef unsigned char quint8;      /* 8 bit unsigned */
typedef short qint16;              /* 16 bit signed */
typedef unsigned short quint16;    /* 16 bit unsigned */
typedef int qint32;                /* 32 bit signed */
typedef unsigned int quint32;      /* 32 bit unsigned */

// magic number for the file
static const int MagicLength = 16;
static const unsigned char magic[MagicLength] =
{
    0x3c, 0xb8, 0x64, 0x18, 0xca, 0xef, 0x9c, 0x95,
    0xcd, 0x21, 0x1c, 0xbf, 0x60, 0xa1, 0xbd, 0xdd
};

enum
{
    Q_EQ          = 0x01,
    Q_LT          = 0x02,
    Q_LEQ         = 0x03,
    Q_BETWEEN     = 0x04,

    Q_NOT         = 0x08,
    Q_MOD_10      = 0x10,
    Q_MOD_100     = 0x20,
    Q_LEAD_1000   = 0x40,

    Q_AND         = 0xFD,
    Q_OR          = 0xFE,
    Q_NEWRULE     = 0xFF,

    Q_OP_MASK     = 0x07,

    Q_NEQ         = Q_NOT | Q_EQ,
    Q_GT          = Q_NOT | Q_LEQ,
    Q_GEQ         = Q_NOT | Q_LT,
    Q_NOT_BETWEEN = Q_NOT | Q_BETWEEN
};

struct QTranslatorPrivate
{
    enum { Contexts = 0x2f, Hashes = 0x42, Messages = 0x69, NumerusRules = 0x88, Dependencies = 0x96 };
};

enum Tag { Tag_End = 1, Tag_SourceText16, Tag_Translation, Tag_Context16, Tag_Obsolete1,
           Tag_SourceText, Tag_Context, Tag_Comment, Tag_Obsolete2 };


static unsigned char read8(const unsigned char *data)
{
    return data[0];
}

static unsigned short read16(const unsigned char *data)
{
    return  ((((unsigned short)data[0])<<8) &0xFF00)  |
            ((((unsigned short)data[1])<<0) &0x00FF);//qFromBigEndian<quint16>(data);
}

static unsigned int read32(const unsigned char *data)
{
    return   ((((unsigned int)data[0])<<24)&0xFF000000)  |
             ((((unsigned int)data[1])<<16)&0x00FF0000)  |
             ((((unsigned int)data[2])<<8) &0x0000FF00)  |
             ((((unsigned int)data[3])<<0) &0x000000FF);//qFromBigEndian<quint32>(data);
}


static void elfHash_continue(const char *name, uint &h)
{
    const uchar *k;
    uint g;

    k = (const uchar *) name;
    while (*k) {
        h = (h << 4) + *k++;
        if ((g = (h & 0xf0000000)) != 0)
            h ^= g >> 24;
        h &= ~g;
    }
}

static void elfHash_finish(uint &h)
{
    if (!h)
        h = 1;
}

static uint elfHash(const char *name)
{
    uint hash = 0;
    elfHash_continue(name, hash);
    elfHash_finish(hash);
    return hash;
}


static bool match(const uchar *found, uint foundLen, const char *target, uint targetLen)
{
    // catch the case if \a found has a zero-terminating symbol and \a len includes it.
    // (normalize it to be without the zero-terminating symbol)
    if (foundLen > 0 && found[foundLen-1] == '\0')
        --foundLen;
    return ((targetLen == foundLen) && memcmp(found, target, foundLen) == 0);
}


/*
   \internal

   Determines whether \a rules are valid "numerus rules". Test input with this
   function before calling numerusHelper, below.
 */
static bool isValidNumerusRules(const unsigned char *rules, unsigned int rulesSize)
{
    // Disabled computation of maximum numerus return value
    // quint32 numerus = 0;

    if (rulesSize == 0)
        return true;

    unsigned int offset = 0;
    do {
        unsigned char opcode = rules[offset];
        unsigned char op = opcode & Q_OP_MASK;

        if (opcode & 0x80)
            return false; // Bad op

        if (++offset == rulesSize)
            return false; // Missing operand

        // right operand
        ++offset;

        switch (op)
        {
        case Q_EQ:
        case Q_LT:
        case Q_LEQ:
            break;

        case Q_BETWEEN:
            if (offset != rulesSize) {
                // third operand
                ++offset;
                break;
            }
            return false; // Missing operand

        default:
            return false; // Bad op (0)
        }

        // ++numerus;
        if (offset == rulesSize)
            return true;

    } while (((rules[offset] == Q_AND)
                || (rules[offset] == Q_OR)
                || (rules[offset] == Q_NEWRULE))
            && ++offset != rulesSize);

    // Bad op
    return false;
}


/*
   \internal

   This function does no validation of input and assumes it is well-behaved,
   these assumptions can be checked with isValidNumerusRules, above.

   Determines which translation to use based on the value of \a n. The return
   value is an index identifying the translation to be used.

   \a rules is a character array of size \a rulesSize containing bytecode that
   operates on the value of \a n and ultimately determines the result.

   This function has O(1) space and O(rulesSize) time complexity.
 */
static unsigned int numerusHelper(int n, const unsigned char *rules, unsigned int rulesSize)
{
    unsigned int result = 0;
    unsigned int i = 0;

    if (rulesSize == 0)
        return 0;

    for (;;) {
        bool orExprTruthValue = false;

        for (;;) {
            bool andExprTruthValue = true;

            for (;;) {
                bool truthValue = true;
                int opcode = rules[i++];

                int leftOperand = n;
                if (opcode & Q_MOD_10) {
                    leftOperand %= 10;
                } else if (opcode & Q_MOD_100) {
                    leftOperand %= 100;
                } else if (opcode & Q_LEAD_1000) {
                    while (leftOperand >= 1000)
                        leftOperand /= 1000;
                }

                int op = opcode & Q_OP_MASK;
                int rightOperand = rules[i++];

                switch (op) {
                case Q_EQ:
                    truthValue = (leftOperand == rightOperand);
                    break;
                case Q_LT:
                    truthValue = (leftOperand < rightOperand);
                    break;
                case Q_LEQ:
                    truthValue = (leftOperand <= rightOperand);
                    break;
                case Q_BETWEEN:
                    int bottom = rightOperand;
                    int top = rules[i++];
                    truthValue = (leftOperand >= bottom && leftOperand <= top);
                }

                if (opcode & Q_NOT)
                    truthValue = !truthValue;

                andExprTruthValue = andExprTruthValue && truthValue;

                if (i == rulesSize || rules[i] != Q_AND)
                    break;
                ++i;
            }

            orExprTruthValue = orExprTruthValue || andExprTruthValue;

            if (i == rulesSize || rules[i] != Q_OR)
                break;
            ++i;
        }

        if (orExprTruthValue)
            return result;

        ++result;

        if (i == rulesSize)
            return result;

        i++; // Q_NEWRULE
    }

    //Q_ASSERT(false);
    assert(false);
    return 0;
}

static std::wstring getMessage(const unsigned char *m, const unsigned char *end, const char *context,
                               const char *sourceText, const char *comment, unsigned int numerus)
{
#ifdef QMTRANSLATPR_DEEP_DEBUG
    printf("-----> Try take message...!\n");
#endif

    const uchar *tn = 0;
    uint tn_length = 0;
    const uint sourceTextLen = uint(strlen(sourceText));
    const uint contextLen = uint(strlen(context));
    const uint commentLen = uint(strlen(comment));

    for (;;) {
        unsigned char tag = 0;
        if (m < end)
            tag = read8(m++);
        switch((Tag)tag)
        {
        case Tag_End:
            goto end;
        case Tag_Translation: {
            int len = read32(m);
            if (len % 1)
                return std::wstring();
            m += 4;
            if (!numerus--) {
                tn_length = len;
                tn = m;
            }
            m += len;
            break;
        }
        case Tag_Obsolete1:
            m += 4;
            break;
        case Tag_SourceText: {
            quint32 len = read32(m);
            m += 4;
            if (!match(m, len, sourceText, sourceTextLen))
            {
                #ifdef QMTRANSLATPR_DEEP_DEBUG
                printf("-----> Source text doesn't match!\n");
                #endif
                return std::wstring();
            }
            m += len;
        }
            break;
        case Tag_Context: {
            quint32 len = read32(m);
            m += 4;
            if (!match(m, len, context, contextLen))
            {
                #ifdef QMTRANSLATPR_DEEP_DEBUG
                printf("-----> Tag gontext doesn't match!\n");
                #endif
                return std::wstring();
            }
            m += len;
        }
            break;
        case Tag_Comment: {
            quint32 len = read32(m);
            m += 4;
            if (*m && !match(m, len, comment, commentLen))
                return std::wstring();
            m += len;
        }
            break;
        default:
            #ifdef QMTRANSLATPR_DEEP_DEBUG
            printf("-----> Wrong tag!\n");
            #endif
            return std::wstring();
        }
    }
end:
    if (!tn)
    {
        #ifdef QMTRANSLATPR_DEEP_DEBUG
        printf("-----> Empty TN!\n");
        #endif
        return std::wstring();
    }
    #ifdef QMTRANSLATPR_DEEP_DEBUG
    printf("-----> Almost got...!\n");
    #endif

    UTF16*   utf16str = (UTF16*)tn;
    intptr_t utf16str_len = tn_length/2;

    unsigned int aVal = 0x11223344;
    char * myValReadBack = (char *)(&aVal);
    if(*myValReadBack != 0x11)//if little endian detected!
    {
        for(int i=0; i<utf16str_len;i++)
            utf16str[i] = ((utf16str[i]>>8)&0x00FF) + ((utf16str[i]<<8)&0xFF00);
    }

    std::wstring str;
    if(sizeof(wchar_t)==sizeof(UTF32))
    {
        #ifdef QMTRANSLATPR_DEEP_DEBUG
        printf ("wchar_t is UTF32!!!\n");
        #endif

        wchar_t *utf32str = (wchar_t*)malloc((utf16str_len+1)*sizeof(wchar_t));
        memset(utf32str, 0, utf16str_len*sizeof(wchar_t));
        const UTF16 * pUtf16 = (const UTF16 *) utf16str;
              UTF32 * pUtf32 = (UTF32*)utf32str;
        ConvertUTF16toUTF32( &pUtf16, pUtf16+utf16str_len,
                             &pUtf32, pUtf32+utf16str_len, lenientConversion);
        //str = std::wstring(utf32str, utf16str_len);
        str.append(utf32str, utf16str_len);
        free(utf32str);
    } else {
        #ifdef QMTRANSLATPR_DEEP_DEBUG
        printf("wchar_t is UTF16!!!\n");
        #endif
        str.append((wchar_t*)utf16str, utf16str_len);
    }

    #ifdef QMTRANSLATPR_DEEP_DEBUG
    for(int i=0;i<utf16str_len;i++)
    {
        wchar_t wch = str[i];
        printf("%08X ", (int)wch);
    }
    printf("\n");
    #endif
//        #ifdef QMTRANSLATPR_DEEP_DEBUG
//        printf("-----> Swap endians... %lu!\n", sizeof(wchar_t));
//        #endif
//        for (unsigned int i = 0; i < str.size(); ++i)
//            str[i] = ((str[i]>>24)&0xff) + ((str[i] << 24) & 0xff000000) + ((str[i] << 16) & 0xff0000) + ((str[i] << 8) & 0xff00);
//    }
    return str;
}


QmTranslatorX::QmTranslatorX()
{
    close();
}

QmTranslatorX::~QmTranslatorX()
{
    if(FileData)
        free(FileData);

}

std::wstring QmTranslatorX::do_translate(const char *context, const char *sourceText, const char *comment, int n)
{
    if (context == 0)
        context = "";
    if (sourceText == 0)
        sourceText = "";
    if (comment == 0)
        comment = "";

    unsigned int numerus = 0;
    size_t numItems = 0;

    if (!offsetLength)
    {
        #ifdef QMTRANSLATPR_DEEP_DEBUG
        printf("--> ZERO OFFSETS LENGTH!");
        #endif
        goto searchDependencies;
    }

    /*
        Check if the context belongs to this QTranslator. If many
        translators are installed, this step is necessary.
    */
    if (contextLength) {
        #ifdef QMTRANSLATPR_DEEP_DEBUG
        printf("--> Finding contexts...!");
        #endif
        unsigned short hTableSize = read16(contextArray);
        unsigned int g = elfHash(context) % hTableSize;
        const unsigned char *c = contextArray + 2 + (g << 1);
        unsigned short off = read16(c);
        c += 2;
        if (off == 0)
        {
            #ifdef QMTRANSLATPR_DEEP_DEBUG
            printf("--> Zero offset...!\n");
            #endif
            return std::wstring();
        }
        c = contextArray + (2 + (hTableSize << 1) + (off << 1));

        const uint contextLen = uint(strlen(context));
        for (;;) {
            unsigned char len = read8(c++);
            if (len == 0)
            {
                #ifdef QMTRANSLATPR_DEEP_DEBUG
                printf("--> Zero length...!\n");
                #endif
                return std::wstring();
            }
            if (match(c, len, context, contextLen))
                break;
            c += len;
        }
    } else {
        #ifdef QMTRANSLATPR_DEEP_DEBUG
        printf("--> Contexts are empty!\n");
        #endif
    }

    numItems = offsetLength / (2 * sizeof(unsigned));
    if (!numItems)
    {
        #ifdef QMTRANSLATPR_DEEP_DEBUG
        printf("--> NO ITEMS!\n");
        #endif
        goto searchDependencies;
    }

    if (n >= 0)
        numerus = numerusHelper(n, numerusRulesArray, numerusRulesLength);

    for (;;) {
        unsigned int h = 0;
        elfHash_continue(sourceText, h);
        elfHash_continue(comment, h);
        elfHash_finish(h);

        const unsigned char *start = offsetArray;
        const unsigned char *end = start + ((numItems-1) << 3);
        while (start <= end) {
            const unsigned char *middle = start + (((end - start) >> 4) << 3);
            unsigned int hash = read32(middle);
            if (h == hash) {
                start = middle;
                break;
            } else if (hash < h) {
                start = middle + 8;
            } else {
                end = middle - 8;
            }
        }

        if (start <= end) {
            // go back on equal key
            while (start != offsetArray && read32(start) == read32(start-8))
                start -= 8;

            while (start < offsetArray + offsetLength) {
                quint32 rh = read32(start);
                start += 4;
                if (rh != h)
                    break;
                quint32 ro = read32(start);
                start += 4;
                std::wstring tn = getMessage(messageArray + ro, messageArray + messageLength, context,
                                             sourceText, comment, numerus);
                if (!tn.empty())
                    return tn;
            }
        }
        if (!comment[0])
            break;
        comment = "";
    }

    #ifdef QMTRANSLATPR_DEEP_DEBUG
    printf("--> Nothing found!\n");
    #endif
searchDependencies:
    #ifdef QMTRANSLATPR_DEEP_DEBUG
    printf("--> Dependencies doesn't implemented, go away...!\n");
    #endif
    /*
    for (QTranslator *translator : subTranslators) {
        QString tn = translator->translate(context, sourceText, comment, n);
        if (!tn.isNull())
            return tn;
    }*/
    return std::wstring();
}

bool QmTranslatorX::loadFile(const char *filePath)
{
    unsigned char magicBuffer[MagicLength];

    FILE* file = fopen(filePath, "rb");
    if(!file)
        return false;//err("Can't open file!", 2);

    if( fread(magicBuffer, 1, MagicLength, file) < MagicLength )
    {
        fclose(file);
        return false;//err("ERROR READING MAGIC NUMBER!!!", 3);
    }

    if( memcmp(magicBuffer, magic, MagicLength) )
    {
        fclose(file);
        return false;//err("MAGIC NUMBER DOESN'T CASE!", 4);
    }

    fseek(file, 0L, SEEK_END);
    FileLength = ftell(file);
    fseek(file, 0L, SEEK_SET);

    FileData = (unsigned char*)malloc(FileLength);
    fread(FileData, 1, FileLength, file);
    fclose(file);
    loadData(FileData, FileLength);
    return true;
}

bool QmTranslatorX::loadData(unsigned char *data, int len)
{
    std::vector<std::string> dependencies;
    bool ok = true;
    const unsigned char *end = data + len;

    data += MagicLength;
    while (data < end - 4)
    {
        unsigned char tag = read8(data++);
        unsigned int blockLen = read32(data);
        data += 4;
        if (!tag || !blockLen)
            break;
        if ((unsigned int)(end - data) < blockLen) {
            ok = false;
            break;
        }

        if (tag == QTranslatorPrivate::Contexts) {
            contextArray = data;
            contextLength = blockLen;
            #ifdef QMTRANSLATPR_DEEP_DEBUG
            printf("Has contexts array!\n");
            #endif
        } else if (tag == QTranslatorPrivate::Hashes) {
            offsetArray = data;
            offsetLength = blockLen;
            #ifdef QMTRANSLATPR_DEEP_DEBUG
            printf("Has hashes! %i\n", offsetLength);
            #endif
        } else if (tag == QTranslatorPrivate::Messages) {
            messageArray = data;
            messageLength = blockLen;
            #ifdef QMTRANSLATPR_DEEP_DEBUG
            printf("Has messages! %i\n", messageLength);
            #endif
        } else if (tag == QTranslatorPrivate::NumerusRules) {
            numerusRulesArray = data;
            numerusRulesLength = blockLen;
            #ifdef QMTRANSLATPR_DEEP_DEBUG
            printf("Has numerus rules! %i\n", numerusRulesLength);
            #endif
        } else if (tag == QTranslatorPrivate::Dependencies) {

            std::string dep;
            while(blockLen != 0)
            {
                unsigned char* begin = data;
                while(*data != '\0') { data++; }
                int gotLen = (data-begin);
                dep = std::string((char*)begin, gotLen);
                if(dep.size()>0)
                {
                    //List of dependent files
                    dependencies.push_back(dep);
                    #ifdef QMTRANSLATPR_DEEP_DEBUG
                    printf("Dependency: %s\n", dep.c_str());
                    #endif
                }
            }
            #ifdef QMTRANSLATPR_DEEP_DEBUG
            printf("Had deps!\n");
            #endif
            /*

            QDataStream stream(QByteArray::fromRawData((const char*)data, blockLen));
            QString dep;
            while (!stream.atEnd()) {
                stream >> dep;
                dependencies.append(dep);
            }*/

        }
        data += blockLen;
    }


    if (dependencies.empty() && (!offsetArray || !messageArray))
        ok = false;
    #ifdef QMTRANSLATPR_DEEP_DEBUG
    printf("Dependencies valid: %i\n", ok);
    #endif

    if (ok && !isValidNumerusRules(numerusRulesArray, numerusRulesLength))
        ok = false;

    #ifdef QMTRANSLATPR_DEEP_DEBUG
    printf("Numerus rules valid: %i\n", ok);
    #endif

    /* //Process loading of sub-translators
    if (ok) {
        const int dependenciesCount = dependencies.count();
        subTranslators.reserve(dependenciesCount);
        for (int i = 0 ; i < dependenciesCount; ++i) {
            QTranslator *translator = new QTranslator;
            subTranslators.append(translator);
            ok = translator->load(dependencies.at(i), directory);
            if (!ok)
                break;
        }

        // In case some dependencies fail to load, unload all the other ones too.
        if (!ok) {
            qDeleteAll(subTranslators);
            subTranslators.clear();
        }
    }*/

    if (!ok) {
        messageArray = 0;
        contextArray = 0;
        offsetArray = 0;
        numerusRulesArray = 0;
        messageLength = 0;
        contextLength = 0;
        offsetLength = 0;
        numerusRulesLength = 0;
        #ifdef QMTRANSLATPR_DEEP_DEBUG
        printf("LOADING FAILED!\n");
        #endif
        return false;
    } else {
        #ifdef QMTRANSLATPR_DEEP_DEBUG
        printf("LOADING PASSED!\n");
        #endif
        return true;
    }
}

void QmTranslatorX::close()
{
    messageArray = 0;
    contextArray = 0;
    offsetArray = 0;
    numerusRulesArray = 0;
    messageLength = 0;
    contextLength = 0;
    offsetLength = 0;
    numerusRulesLength = 0;
    if(FileData)
        free(FileData);
    FileData = 0;
    FileLength = 0;
}
