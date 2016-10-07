# QTranslator-STL
STL-compatible implementation of Qt Translator core which returns translated string from source text or from trID key

Gives ability to use precompiled qm-translations with non-Qt projects.

Useful for a small console applications or for graphical applications that prefered to use SDL, OpenGL, DirecX, etc. than GUI and has small size.

This implementation was created with using the code of original Qt-Translator which processes qm-files which has been ported to pure STL.

# Main features
* It's tiny
* Easy to use
* No dependencies
* You still be able to use a power of the Qt Lingust!

# TODO
* Implement QObject::tr and qtTrId surrogates to process translatable strings and keep compatibility with lupdate utility
* Implement support of "dependent translations" code (which wasn't done and commented yet)
