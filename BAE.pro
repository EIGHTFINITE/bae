###############################
## BUILD OPTIONS
###############################

TEMPLATE = app
TARGET = bae

QT += core gui widgets

CONFIG += c++11 zlib lz4

INCLUDEPATH += src lib

# Debug/Release options
CONFIG(debug, debug|release) {
	# Debug Options
	BUILD = debug
	CONFIG += console
} else {
	# Release Options
	BUILD = release
	CONFIG -= console
	DEFINES += QT_NO_DEBUG_OUTPUT
}

INTERMEDIATE = $${OUT_PWD}/GeneratedFiles/$${BUILD}

UI_DIR = $${INTERMEDIATE}/.ui
MOC_DIR = $${INTERMEDIATE}/.moc
RCC_DIR = $${INTERMEDIATE}/.qrc
OBJECTS_DIR = $${INTERMEDIATE}/.obj

###############################
## PROJECT SCOPES
###############################

SOURCES += \
	src/main.cpp \
	src/mainwindow.cpp \
	src/archive.cpp \
	src/bsa.cpp \
	src/progressdialog.cpp

HEADERS += \
	src/mainwindow.h \
	src/archive.h \
	src/bsa.h \
	src/progressdialog.h \
	lib/dds.h

FORMS += ui/mainwindow.ui \
	ui/progressdialog.ui


###############################
## DEPENDENCY SCOPES
###############################

zlib {
	INCLUDEPATH += lib/zlib

	HEADERS += \
		lib/zlib/crc32.h \
		lib/zlib/deflate.h \
		lib/zlib/gzguts.h \
		lib/zlib/inffast.h \
		lib/zlib/inffixed.h \
		lib/zlib/inflate.h \
		lib/zlib/inftrees.h \
		lib/zlib/trees.h \
		lib/zlib/zconf.h \
		lib/zlib/zlib.h \
		lib/zlib/zutil.h

	SOURCES += \
		lib/zlib/adler32.c \
		lib/zlib/compress.c \
		lib/zlib/crc32.c \
		lib/zlib/deflate.c \
		lib/zlib/gzclose.c \
		lib/zlib/gzlib.c \
		lib/zlib/gzread.c \
		lib/zlib/gzwrite.c \
		lib/zlib/infback.c \
		lib/zlib/inffast.c \
		lib/zlib/inflate.c \
		lib/zlib/inftrees.c \
		lib/zlib/trees.c \
		lib/zlib/uncompr.c \
		lib/zlib/zutil.c
}

lz4 {
        INCLUDEPATH += lib/lz4

        HEADERS += \
                lib/lz4/lz4.h \
                lib/lz4/lz4frame.h \
                lib/lz4/lz4frame_static.h \
                lib/lz4/lz4hc.h \
                lib/lz4/xxhash.h

        SOURCES += \
                lib/lz4/lz4.c \
                lib/lz4/lz4frame.c \
                lib/lz4/lz4hc.c \
                lib/lz4/xxhash.c
}

###############################
## COMPILER SCOPES
###############################

*msvc* {
	# So VCProj Filters do not flatten headers/source
	CONFIG -= flat

	# COMPILER FLAGS

	#  Optimization flags
	#QMAKE_CXXFLAGS_RELEASE *= -O2
	#  Multithreaded compiling for Visual Studio
	#QMAKE_CXXFLAGS += -MP
}

*-g++ {

	# COMPILER FLAGS

	#  Optimization flags
	QMAKE_CXXFLAGS_DEBUG -= -O0 -g
	QMAKE_CXXFLAGS_DEBUG *= -Og -g3
	QMAKE_CXXFLAGS_RELEASE *= -O3 -mfpmath=sse

	# C++11 Support
	QMAKE_CXXFLAGS_RELEASE *= -std=c++11

	#  Extension flags
	QMAKE_CXXFLAGS_RELEASE *= -msse2 -msse
}
