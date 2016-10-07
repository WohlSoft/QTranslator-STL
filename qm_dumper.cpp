#include <stdio.h>
#include <string>
#include <memory.h>
#include <iostream>

#include "QTranslatorX/QTranslatorX"

//Globally declared translator
QmTranslatorX translator;

/**
 * @brief Example of fake qtTrId used for ID-based translations
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
    printf("\n%s\n", errMsg);
    return code;
}

int main(int argc, char**argv)
{
    if(argc<=1)
        return err("Missing argument! [must be path to file which need to dump]!", 1);

    if(!translator.loadFile(argv[1]))
        return err("Can't load translation!", 1);


    std::cout << "Testing translations in work:\n";

                 //% "Just a some testing string"
    std::cout << "test 1 (tr-ID): " << qtTrId("SomethingID1") << "\n";
                 //% "Another testing string"
    std::cout << "test 2 (tr-ID): " << qtTrId("SomethingID2") << "\n";

    std::cout << "test 3 (\"Fake\" context): " << Fake::tr("Another testing string for some", "Please do accurate translation and never produce shit!") << "\n";
    std::cout << "test 4 (\"Fake\" context): " << Fake::tr("What the heck you still try translate me?!", "Do you like jokes? Translate in Goblin style!") << "\n";

    return 0;
}


