#include <stdio.h>
#include <string>
#include <memory.h>

#include "qm_translator.h"
#include "ConvertUTF.h"

int err(const char* errMsg, int code)
{
    printf("\n%s\n", errMsg);
    return code;
}

void testPhraze(QmTranslatorX& tr, const char* phraze)
{
    std::wstring test = tr.do_translate(0, phraze, 0, -1);

    int utf32len = test.size();
    int utf8len = test.size()*sizeof(wchar_t)+1;
    char* utf8str = (char*)malloc(utf8len);
    memset(utf8str, 0, utf8len);
    if(utf32len>0)
    {
        wchar_t* utf32s= (wchar_t*)test.data();
        if(sizeof(wchar_t) == sizeof(UTF32))
        {
            const UTF32 * pUtf32 = (const UTF32*)utf32s;
                  UTF8  * pUtf8  = (UTF8*)utf8str;
            ConvertUTF32toUTF8( &pUtf32, pUtf32+test.size(),
                                &pUtf8,  pUtf8+test.size()*4, lenientConversion );
        } else {
            const UTF16 * pUtf16 = (const UTF16*)utf32s;
                  UTF8  * pUtf8  = (UTF8*)utf8str;
            ConvertUTF16toUTF8( &pUtf16, pUtf16+test.size(),
                                &pUtf8,  pUtf8+ test.size()*4, lenientConversion );
        }
    }

    FILE* out = fopen("ouch.txt", "a");

    printf("Small test: (length %d) {%s} \n", test.size(), utf8str );
    fflush(stdout);
    fprintf(out, "Small test: (length %d) {%s} \n", test.size(), utf8str );
    fflush(out);
    fclose(out);
    free(utf8str);
}

int main(int argc, char**argv)
{
    if(argc<=1)
        return err("Missing argument! [must be path to file which need to dump]!", 1);
    
    QmTranslatorX tr;
    if(!tr.loadFile(argv[1]))
        return err("Can't load translation!", 1);

    testPhraze(tr, "ERROR_NO_OPEN_FILES_MSG");
    testPhraze(tr, "CRASH_UNHEXC_MSG");
    testPhraze(tr, "FUCKING SHIT");
    testPhraze(tr, "MSGBOX_WARN");
    testPhraze(tr, "LVL_ERROR_LVLCLOSED");
    testPhraze(tr, "ERROR_LVL_UNKNOWN_PL_CHARACTER");
    testPhraze(tr, "LVL_MENU_PAUSE_CONTINUESAVE");
    testPhraze(tr, "MAINMENU_2_PLAYER_GAME");
    testPhraze(tr, "TEST_TEXTINPUTBOX");
    testPhraze(tr, "LVL_ERROR_NOSECTIONS");

    printf("Done!\n");

    return 0;
}


