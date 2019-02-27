## set the QTC_SOURCE environment variable to override the setting here
QTCREATOR_SOURCES = $$(QTC_SOURCE)

## set the QTC_BUILD environment variable to override the setting here
IDE_BUILD_TREE = $$(QTC_BUILD_INPLACE)

DEFINES += CMAKEPROJECTMANAGER_LIBRARY
include(cmakeprojectmanager_dependencies.pri)
include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)

# For the highlighter:
INCLUDEPATH += $$QTCREATOR_SOURCES/src/plugins/texteditor

HEADERS = builddirmanager.h \
    builddirparameters.h \
    builddirreader.h \
    cmakebuildstep.h \
    cmakebuildtarget.h \
    cmakeconfigitem.h \
    cmakeproject.h \
    cmakeprojectimporter.h \
    cmakeprojectplugin.h \
    cmakeprojectmanager.h \
    cmakeprojectconstants.h \
    cmakeprojectnodes.h \
    cmakerunconfiguration.h \
    cmakebuildconfiguration.h \
    cmakeeditor.h \
    cmakelocatorfilter.h \
    cmakefilecompletionassist.h \
    cmaketool.h \
    cmaketoolsettingsaccessor.h \
    cmakeparser.h \
    cmakesettingspage.h \
    cmaketoolmanager.h \
    cmake_global.h \
    cmakekitinformation.h \
    cmakecbpparser.h \
    cmakebuildsettingswidget.h \
    cmakeindenter.h \
    cmakeautocompleter.h \
    cmakespecificsettings.h \
    cmakespecificsettingspage.h \
    configmodel.h \
    configmodelitemdelegate.h \
    servermode.h \
    servermodereader.h \
    tealeafreader.h \
    treescanner.h \
    compat.h \
    simpleservermodereader.h

SOURCES = builddirmanager.cpp \
    builddirparameters.cpp \
    builddirreader.cpp \
    cmakebuildstep.cpp \
    cmakebuildtarget.cpp \
    cmakeconfigitem.cpp \
    cmakeproject.cpp \
    cmakeprojectimporter.cpp \
    cmakeprojectplugin.cpp \
    cmakeprojectmanager.cpp \
    cmakeprojectnodes.cpp \
    cmakerunconfiguration.cpp \
    cmakebuildconfiguration.cpp \
    cmakeeditor.cpp \
    cmakelocatorfilter.cpp \
    cmakefilecompletionassist.cpp \
    cmaketool.cpp \
    cmaketoolsettingsaccessor.cpp \
    cmakeparser.cpp \
    cmakesettingspage.cpp \
    cmaketoolmanager.cpp \
    cmakekitinformation.cpp \
    cmakecbpparser.cpp \
    cmakebuildsettingswidget.cpp \
    cmakeindenter.cpp \
    cmakeautocompleter.cpp \
    cmakespecificsettings.cpp \
    cmakespecificsettingspage.cpp \
    configmodel.cpp \
    configmodelitemdelegate.cpp \
    servermode.cpp \
    servermodereader.cpp \
    tealeafreader.cpp \
    treescanner.cpp \
    compat.cpp \
    simpleservermodereader.cpp

RESOURCES += cmakeproject.qrc

FORMS += \
    cmakespecificsettingspage.ui

OTHER_FILES += README.txt

WIZARD_FILES = wizard/cmake2/*

wizardfiles.files = $$WIZARD_FILES
wizardfiles.path = $$QTC_PREFIX/share/qtcreator/templates/wizards/projects/cmake2/
wizardfiles.CONFIG += no_check_exist
INSTALLS += wizardfiles
