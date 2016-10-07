# QTranslator-STL
STL-compatible implementation of Qt Translator core which returns translated string from source text or from trID key

Gives ability to use precompiled qm-translations with non-Qt projects.

Useful for a small console applications or for graphical applications that prefered to use SDL, OpenGL, DirecX, etc. than GUI and has small size.

This implementation was created with using the code of original Qt-Translator which processes qm-files which has been ported to pure STL.

**Note:** You still need use QMake-based project to allow lupdate correctly dig source files and spawn translation files.
However you can process translations update without using QMake-based project, just by manual specifying source to scan (files, directories, file lists, etc.) and targets to generate: `lupdate cpp1.cpp cpp2.cpp -ts cpp_en.ts cpp_ru.ts`. 
More detail guide you can get by typing `lupdate --help`. Also possible to redefine names of translating functions!

**Why license is GPL, not a MIT?** This implementation uses some of internal Qt code (qtbase/src/corelib/kernel/qtranslator.cpp) which licensed under GNU GPL license. 
If you wish use this in proprietary projects, you have to buy commercial Qt license from Digia, or if you already have Qt license, feel free to use this class everywhere you want.

# Main features
* It's tiny
* Easy to use
* No dependencies (even Qt itself doesn't needed for runtime, but lupdate, lrelease and linguist are needed to work with translations)
* You still be able to use a power of the Qt Lingust!
* Allows you have alone global translator without having different translators in different places
* Gives you a freedom for choisin output string format of tr and qtTrId functions

# How to install
* Copy **QTranslatorX** folder into your project directory
* Enable C++11 support if not enabled

# Example of usage (Tr-ID based)
```C++
#include <string>
#include "QTranslatorX/QTranslatorX"

//! Globally defined translator
QmTranslatorX translator;

/**
 * @brief Example of fake qtTrId used for ID-based translations. To correctly compile tr-ID-based qm-file you must use -idbased flag for lrelease utility
 * @param trSrc source string
 * @return translated UTF8 string
 */
std::string qtTrId(const char* trSrc)
{
    std::string out = translator.do_translate8(0, trSrc, 0, -1);
    if(out.empty())
        return std::string(trSrc);
    else
        return out;
}

int err(const char* errMsg, int code)
{
    std::cout << "\n" << errMsg << "\n";
    return code;
}

///
/// Opens qm-file from argument and prints translated phrase
///
int main(int argc, char**argv)
{
    if(argc<=1)
        return err("Missing argument! [must be a path to compiled translation file!]", 1);

    if(!translator.loadFile(argv[1]))
        return err("Can't load translation!", 1);

                 //% "Hello international world!"
    std::cout << qtTrId("HelloWorldID") << "\n";

    return 0;
}
```


# Example of usage (Class-name context based)

```C++

#include <string>
#include "QTranslatorX/QTranslatorX"

//! Globally defined translator
QmTranslatorX translator;

/**
 * @brief Example of the class with TR function (to make context-based non-ID based translations)
 */
class Fake
{
    //Fake Q_OBJECT macro to avoid "Class 'Fake' lacks Q_OBJECT macro" spawned from lupdate utility
    #define Q_OBJECT
    Q_OBJECT
    #undef Q_OBJECT

public:
    /**
     * @brief Translating function of context-based translation.
     * @param trSrc source string
     * @return translated string
     *
     * Make own class which will contain tr(const char*) or tr(const char*, const char*=0) (wuth developer comments support) function\
     * Output you can provide any: std::string, std::u16string, std::u32string or others like std::wstring and any others
     */
    static std::string tr(const char* trSrc, const char* /*Developer comment*/ = 0)
    {
        std::string out = translator.do_translate8("Fake", trSrc, 0, -1);
        if(out.empty())
            return std::string(trSrc);
        else
            return out;
    }
};

int err(const char* errMsg, int code)
{
    std::cout << "\n" << errMsg << "\n";
    return code;
}

///
/// Opens qm-file from argument and prints translated phrase
///
int main(int argc, char**argv)
{
    if(argc<=1)
        return err("Missing argument! [must be a path to compiled translation file!]", 1);

    if(!translator.loadFile(argv[1]))
        return err("Can't load translation!", 1);

    std::cout << Fake::tr("Hello international world!") << "\n";

    return 0;
}

```

