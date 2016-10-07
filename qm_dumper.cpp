#include <stdio.h>
#include <string>
#include <memory.h>

#include "qm_translator.h"

int err(const char* errMsg, int code)
{
    printf("\n%s\n", errMsg);
    return code;
}

void testPhraze(QmTranslatorX& tr, const char* phraze, FILE* out)
{
    std::u32string str32 = tr.do_translate32(0, phraze, 0, -1);
    std::string    strU8 = tr.do_translate8(0, phraze, 0, -1);
    printf("Small test: (length %d) {%s} \n", (int)str32.size(), strU8.c_str() );
    fflush(stdout);
    fprintf(out, "Small test: (length %d) {%s} \n", (int)str32.size(), strU8.c_str() );
    fflush(out);
}

int main(int argc, char**argv)
{
    if(argc<=1)
        return err("Missing argument! [must be path to file which need to dump]!", 1);
    
    QmTranslatorX tr;
    if(!tr.loadFile(argv[1]))
        return err("Can't load translation!", 1);

    FILE* out = fopen("result.txt", "w");
    char bom[4] = "\xEF\xBB\xBF";
    fwrite(bom, 1, 3, out);

    testPhraze(tr, "ERROR_NO_OPEN_FILES_MSG", out);
    testPhraze(tr, "CRASH_UNHEXC_MSG", out);
    testPhraze(tr, "FUCKING SHIT", out); //<-- Example of non-existing string
    testPhraze(tr, "MSGBOX_WARN", out);
    testPhraze(tr, "LVL_ERROR_LVLCLOSED", out);
    testPhraze(tr, "ERROR_LVL_UNKNOWN_PL_CHARACTER", out);
    testPhraze(tr, "LVL_MENU_PAUSE_CONTINUESAVE", out);
    testPhraze(tr, "MAINMENU_2_PLAYER_GAME", out);
    testPhraze(tr, "TEST_TEXTINPUTBOX", out);
    testPhraze(tr, "LVL_ERROR_NOSECTIONS", out);

    fclose(out);

    printf("Done!\n");

    return 0;
}


