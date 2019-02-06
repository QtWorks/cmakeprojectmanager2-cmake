/****************************************************************************
**
** Copyright (C) 2016 Canonical Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cmakekitinformation.h"
#include "cmakekitconfigwidget.h"
#include "cmakeprojectconstants.h"
#include "cmaketoolmanager.h"
#include "cmaketool.h"

#include <app/app_version.h>
#include <projectexplorer/task.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QVariant>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
// --------------------------------------------------------------------
// CMakeKitAspect:
// --------------------------------------------------------------------

static Core::Id defaultCMakeToolId()
{
    CMakeTool *defaultTool = CMakeToolManager::defaultCMakeTool();
    return defaultTool ? defaultTool->id() : Core::Id();
}

static const char TOOL_ID[] = "CMakeProjectManager.CMakeKitInformation";

// --------------------------------------------------------------------
// CMakeKitAspect:
// --------------------------------------------------------------------

CMakeKitAspect::CMakeKitAspect()
{
    setObjectName(QLatin1String("CMakeKitAspect"));
    setId(TOOL_ID);
    setPriority(20000);

    //make sure the default value is set if a selected CMake is removed
    connect(CMakeToolManager::instance(), &CMakeToolManager::cmakeRemoved,
            [this]() { foreach (Kit *k, KitManager::kits()) fix(k); });

    //make sure the default value is set if a new default CMake is set
    connect(CMakeToolManager::instance(), &CMakeToolManager::defaultCMakeChanged,
            [this]() { foreach (Kit *k, KitManager::kits()) fix(k); });
}

Core::Id CMakeKitAspect::id()
{
    return TOOL_ID;
}

Core::Id CMakeKitAspect::cmakeToolId(const Kit *k)
{
    if (!k)
        return {};
    return Core::Id::fromSetting(k->value(TOOL_ID));
}

CMakeTool *CMakeKitAspect::cmakeTool(const Kit *k)
{
    return CMakeToolManager::findById(cmakeToolId(k));
}

void CMakeKitAspect::setCMakeTool(Kit *k, const Core::Id id)
{
    const Core::Id toSet = id.isValid() ? id : defaultCMakeToolId();
    QTC_ASSERT(!id.isValid() || CMakeToolManager::findById(toSet), return);
    if (k)
        k->setValue(TOOL_ID, toSet.toSetting());
}

QVariant CMakeKitAspect::defaultValue(const Kit *k) const
{
    const Core::Id id = k ? defaultCMakeToolId() : Core::Id();
    return id.toSetting();
}

QList<Task> CMakeKitAspect::validate(const Kit *k) const
{
    QList<Task> result;
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (tool) {
        CMakeTool::Version version = tool->version();
        if (version.major < 3) {
            result << Task(Task::Warning, tr("CMake version %1 is unsupported. Please update to "
                                             "version 3.0 or later.").arg(QString::fromUtf8(version.fullVersion)),
                           Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }
    return result;
}

void CMakeKitAspect::setup(Kit *k)
{
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (!tool)
        setCMakeTool(k, defaultCMakeToolId());
}

void CMakeKitAspect::fix(Kit *k)
{
    if (!CMakeKitAspect::cmakeTool(k))
        setup(k);
}

KitAspect::ItemList CMakeKitAspect::toUserOutput(const Kit *k) const
{
    const CMakeTool *const tool = cmakeTool(k);
    return ItemList() << qMakePair(tr("CMake"), tool ? tool->displayName() : tr("Unconfigured"));
}

KitAspectWidget *CMakeKitAspect::createConfigWidget(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::CMakeKitAspectWidget(k, this);
}

void CMakeKitAspect::addToMacroExpander(Kit *k, Utils::MacroExpander *expander) const
{
    QTC_ASSERT(k, return);
    expander->registerFileVariables("CMake:Executable", tr("Path to the cmake executable"),
                                    [k]() -> QString {
                                        CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
                                        return tool ? tool->cmakeExecutable().toString() : QString();
    });
}

QSet<Core::Id> CMakeKitAspect::availableFeatures(const Kit *k) const
{
    if (cmakeTool(k))
        return { CMakeProjectManager::Constants::CMAKE_FEATURE_ID };
    return {};
}

// --------------------------------------------------------------------
// CMakeGeneratorKitAspect:
// --------------------------------------------------------------------

static const char GENERATOR_ID[] = "CMake.GeneratorKitInformation";

static const char GENERATOR_KEY[] = "Generator";
static const char EXTRA_GENERATOR_KEY[] = "ExtraGenerator";
static const char PLATFORM_KEY[] = "Platform";
static const char TOOLSET_KEY[] = "Toolset";

namespace {

struct GeneratorInfo {
    QVariant toVariant() const {
        QVariantMap result;
        result.insert(GENERATOR_KEY, generator);
        result.insert(EXTRA_GENERATOR_KEY, extraGenerator);
        result.insert(PLATFORM_KEY, platform);
        result.insert(TOOLSET_KEY, toolset);
        return result;
    }
    void fromVariant(const QVariant &v) {
        const QVariantMap value = v.toMap();

        generator = value.value(GENERATOR_KEY).toString();
        extraGenerator = value.value(EXTRA_GENERATOR_KEY).toString();
        platform = value.value(PLATFORM_KEY).toString();
        toolset = value.value(TOOLSET_KEY).toString();
    }

    QString generator;
    QString extraGenerator;
    QString platform;
    QString toolset;
};

} // namespace

static GeneratorInfo generatorInfo(const Kit *k)
{
    GeneratorInfo info;
    if (!k)
        return info;

    info.fromVariant(k->value(GENERATOR_ID));
    return info;
}

static void setGeneratorInfo(Kit *k, const GeneratorInfo &info)
{
    if (!k)
        return;
    k->setValue(GENERATOR_ID, info.toVariant());
}

CMakeGeneratorKitAspect::CMakeGeneratorKitAspect()
{
    setObjectName(QLatin1String("CMakeGeneratorKitAspect"));
    setId(GENERATOR_ID);
    setPriority(19000);
}

QString CMakeGeneratorKitAspect::generator(const Kit *k)
{
    return generatorInfo(k).generator;
}

QString CMakeGeneratorKitAspect::extraGenerator(const Kit *k)
{
    return generatorInfo(k).extraGenerator;
}

QString CMakeGeneratorKitAspect::platform(const Kit *k)
{
    return generatorInfo(k).platform;
}

QString CMakeGeneratorKitAspect::toolset(const Kit *k)
{
    return generatorInfo(k).toolset;
}

void CMakeGeneratorKitAspect::setGenerator(Kit *k, const QString &generator)
{
    GeneratorInfo info = generatorInfo(k);
    info.generator = generator;
    setGeneratorInfo(k, info);
}

void CMakeGeneratorKitAspect::setExtraGenerator(Kit *k, const QString &extraGenerator)
{
    GeneratorInfo info = generatorInfo(k);
    info.extraGenerator = extraGenerator;
    setGeneratorInfo(k, info);
}

void CMakeGeneratorKitAspect::setPlatform(Kit *k, const QString &platform)
{
    GeneratorInfo info = generatorInfo(k);
    info.platform = platform;
    setGeneratorInfo(k, info);
}

void CMakeGeneratorKitAspect::setToolset(Kit *k, const QString &toolset)
{
    GeneratorInfo info = generatorInfo(k);
    info.toolset = toolset;
    setGeneratorInfo(k, info);
}

void CMakeGeneratorKitAspect::set(Kit *k,
                                       const QString &generator, const QString &extraGenerator,
                                       const QString &platform, const QString &toolset)
{
    GeneratorInfo info = {generator, extraGenerator, platform, toolset};
    setGeneratorInfo(k, info);
}

QStringList CMakeGeneratorKitAspect::generatorArguments(const Kit *k)
{
    QStringList result;
    GeneratorInfo info = generatorInfo(k);
    if (info.generator.isEmpty())
        return result;

    if (info.extraGenerator.isEmpty()) {
        result.append("-G" + info.generator);
    } else {
        result.append("-G" + info.extraGenerator + " - " + info.generator);
    }

    if (!info.platform.isEmpty())
        result.append("-A" + info.platform);

    if (!info.toolset.isEmpty())
        result.append("-T" + info.toolset);

    return result;
}

QVariant CMakeGeneratorKitAspect::defaultValue(const Kit *k) const
{
    QTC_ASSERT(k, return QVariant());

    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (!tool)
        return QVariant();

    const QString extraGenerator = "CodeBlocks";

    QList<CMakeTool::Generator> known = tool->supportedGenerators();
    auto it = std::find_if(known.constBegin(), known.constEnd(),
                           [extraGenerator](const CMakeTool::Generator &g) {
        return g.matches("Ninja", extraGenerator);
    });
    if (it != known.constEnd()) {
        Utils::Environment env = Utils::Environment::systemEnvironment();
        k->addToEnvironment(env);
        const Utils::FileName ninjaExec = env.searchInPath(QLatin1String("ninja"));
        if (!ninjaExec.isEmpty())
            return GeneratorInfo({QString("Ninja"), extraGenerator, QString(), QString()}).toVariant();
    }

    if (Utils::HostOsInfo::isWindowsHost()) {
        // *sigh* Windows with its zoo of incompatible stuff again...
        ToolChain *tc = ToolChainKitAspect::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        if (tc && tc->typeId() == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID) {
            it = std::find_if(known.constBegin(), known.constEnd(),
                              [extraGenerator](const CMakeTool::Generator &g) {
                return g.matches("MinGW Makefiles", extraGenerator);
            });
        } else {
            it = std::find_if(known.constBegin(), known.constEnd(),
                              [extraGenerator](const CMakeTool::Generator &g) {
                return g.matches("NMake Makefiles", extraGenerator)
                        || g.matches("NMake Makefiles JOM", extraGenerator);
            });
        }
    } else {
        // Unix-oid OSes:
        it = std::find_if(known.constBegin(), known.constEnd(),
                          [extraGenerator](const CMakeTool::Generator &g) {
            return g.matches("Unix Makefiles", extraGenerator);
        });
    }
    if (it == known.constEnd())
        it = known.constBegin(); // Fallback to the first generator...
    if (it == known.constEnd())
        return QVariant();

    return GeneratorInfo({it->name, extraGenerator, QString(), QString()}).toVariant();
}

QList<Task> CMakeGeneratorKitAspect::validate(const Kit *k) const
{
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    GeneratorInfo info = generatorInfo(k);

    QList<Task> result;
    if (tool) {
        if (!tool->isValid()) {
            result << Task(Task::Warning, tr("CMake Tool is unconfigured, CMake generator will be ignored."),
                           Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        } else {
            QList<CMakeTool::Generator> known = tool->supportedGenerators();
            auto it = std::find_if(known.constBegin(), known.constEnd(), [info](const CMakeTool::Generator &g) {
                return g.matches(info.generator, info.extraGenerator);
            });
            if (it == known.constEnd()) {
                result << Task(Task::Warning, tr("CMake Tool does not support the configured generator."),
                               Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
            } else {
                if (!it->supportsPlatform && !info.platform.isEmpty()) {
                    result << Task(Task::Warning, tr("Platform is not supported by the selected CMake generator."),
                                   Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
                }
                if (!it->supportsToolset && !info.toolset.isEmpty()) {
                    result << Task(Task::Warning, tr("Toolset is not supported by the selected CMake generator."),
                                   Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
                }
            }
            if (!tool->hasServerMode() && info.extraGenerator != "CodeBlocks") {
                result << Task(Task::Warning, tr("The selected CMake binary has no server-mode and the CMake "
                                                 "generator does not generate a CodeBlocks file. "
                                                 "%1 will not be able to parse CMake projects.")
                               .arg(Core::Constants::IDE_DISPLAY_NAME),
                               Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
            }
        }
    }
    return result;
}

void CMakeGeneratorKitAspect::setup(Kit *k)
{
    GeneratorInfo info;
    info.fromVariant(defaultValue(k));
    setGeneratorInfo(k, info);
}

void CMakeGeneratorKitAspect::fix(Kit *k)
{
    const CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    const GeneratorInfo info = generatorInfo(k);

    if (!tool)
        return;
    QList<CMakeTool::Generator> known = tool->supportedGenerators();
    auto it = std::find_if(known.constBegin(), known.constEnd(),
                           [info](const CMakeTool::Generator &g) {
        return g.matches(info.generator, info.extraGenerator);
    });
    if (it == known.constEnd()) {
        GeneratorInfo dv;
        dv.fromVariant(defaultValue(k));
        setGeneratorInfo(k, dv);
    } else {
        const GeneratorInfo dv = {info.generator, info.extraGenerator,
                                  it->supportsPlatform ? info.platform : QString(),
                                  it->supportsToolset ? info.toolset : QString()};
        setGeneratorInfo(k, dv);
    }
}

void CMakeGeneratorKitAspect::upgrade(Kit *k)
{
    QTC_ASSERT(k, return);

    const QVariant value = k->value(GENERATOR_ID);
    if (value.type() != QVariant::Map) {
        GeneratorInfo info;
        const QString fullName = value.toString();
        const int pos = fullName.indexOf(" - ");
        if (pos >= 0) {
            info.generator = fullName.mid(pos + 3);
            info.extraGenerator = fullName.mid(0, pos);
        } else {
            info.generator = fullName;
        }
        setGeneratorInfo(k, info);
    }
}

KitAspect::ItemList CMakeGeneratorKitAspect::toUserOutput(const Kit *k) const
{
    const GeneratorInfo info = generatorInfo(k);
    QString message;
    if (info.generator.isEmpty()) {
        message = tr("<Use Default Generator>");
    } else {
        message = tr("Generator: %1<br>Extra generator: %2").arg(info.generator).arg(info.extraGenerator);
        if (!info.platform.isEmpty())
            message += "<br/>" + tr("Platform: %1").arg(info.platform);
        if (!info.toolset.isEmpty())
            message += "<br/>" + tr("Toolset: %1").arg(info.toolset);
    }
    return ItemList() << qMakePair(tr("CMake Generator"), message);
}

KitAspectWidget *CMakeGeneratorKitAspect::createConfigWidget(Kit *k) const
{
    return new Internal::CMakeGeneratorKitAspectWidget(k, this);
}

// --------------------------------------------------------------------
// CMakeConfigurationKitAspect:
// --------------------------------------------------------------------

static const char CONFIGURATION_ID[] = "CMake.ConfigurationKitInformation";

static const char CMAKE_C_TOOLCHAIN_KEY[] = "CMAKE_C_COMPILER";
static const char CMAKE_CXX_TOOLCHAIN_KEY[] = "CMAKE_CXX_COMPILER";
static const char CMAKE_QMAKE_KEY[] = "QT_QMAKE_EXECUTABLE";
static const char CMAKE_PREFIX_PATH_KEY[] = "CMAKE_PREFIX_PATH";

CMakeConfigurationKitAspect::CMakeConfigurationKitAspect()
{
    setObjectName(QLatin1String("CMakeConfigurationKitAspect"));
    setId(CONFIGURATION_ID);
    setPriority(18000);
}

CMakeConfig CMakeConfigurationKitAspect::configuration(const Kit *k)
{
    if (!k)
        return CMakeConfig();
    const QStringList tmp = k->value(CONFIGURATION_ID).toStringList();
    return Utils::transform(tmp, &CMakeConfigItem::fromString);
}

void CMakeConfigurationKitAspect::setConfiguration(Kit *k, const CMakeConfig &config)
{
    if (!k)
        return;
    const QStringList tmp = Utils::transform(config, [](const CMakeConfigItem &i) { return i.toString(); });
    k->setValue(CONFIGURATION_ID, tmp);
}

QStringList CMakeConfigurationKitAspect::toStringList(const Kit *k)
{
    QStringList current
            = Utils::transform(CMakeConfigurationKitAspect::configuration(k),
                               [](const CMakeConfigItem &i) { return i.toString(); });
    current = Utils::filtered(current, [](const QString &s) { return !s.isEmpty(); });
    Utils::sort(current);
    return current;
}

void CMakeConfigurationKitAspect::fromStringList(Kit *k, const QStringList &in)
{
    CMakeConfig result;
    foreach (const QString &s, in) {
        const CMakeConfigItem item = CMakeConfigItem::fromString(s);
        if (!item.key.isEmpty())
            result << item;
    }
    setConfiguration(k, result);
}

CMakeConfig CMakeConfigurationKitAspect::defaultConfiguration(const Kit *k)
{
    Q_UNUSED(k);
    CMakeConfig config;
    // Qt4:
    config << CMakeConfigItem(CMAKE_QMAKE_KEY, "%{Qt:qmakeExecutable}");
    // Qt5:
    config << CMakeConfigItem(CMAKE_PREFIX_PATH_KEY, "%{Qt:QT_INSTALL_PREFIX}");

    config << CMakeConfigItem(CMAKE_C_TOOLCHAIN_KEY, "%{Compiler:Executable:C}");
    config << CMakeConfigItem(CMAKE_CXX_TOOLCHAIN_KEY, "%{Compiler:Executable:Cxx}");

    return config;
}

QVariant CMakeConfigurationKitAspect::defaultValue(const Kit *k) const
{
    Q_UNUSED(k);

    // FIXME: Convert preload scripts
    CMakeConfig config = defaultConfiguration(k);
    const QStringList tmp
            = Utils::transform(config, [](const CMakeConfigItem &i) { return i.toString(); });
    return tmp;
}

QList<Task> CMakeConfigurationKitAspect::validate(const Kit *k) const
{
    QTC_ASSERT(k, return QList<Task>());

    const QtSupport::BaseQtVersion *const version = QtSupport::QtKitAspect::qtVersion(k);
    const ToolChain *const tcC = ToolChainKitAspect::toolChain(k, ProjectExplorer::Constants::C_LANGUAGE_ID);
    const ToolChain *const tcCxx = ToolChainKitAspect::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    const CMakeConfig config = configuration(k);

    const bool isQt4 = version && version->qtVersion() < QtSupport::QtVersionNumber(5, 0, 0);
    Utils::FileName qmakePath;
    QStringList qtInstallDirs;
    Utils::FileName tcCPath;
    Utils::FileName tcCxxPath;
    foreach (const CMakeConfigItem &i, config) {
        // Do not use expand(QByteArray) as we cannot be sure the input is latin1
        const Utils::FileName expandedValue
            = Utils::FileName::fromString(k->macroExpander()->expand(QString::fromUtf8(i.value)));
        if (i.key == CMAKE_QMAKE_KEY)
            qmakePath = expandedValue;
        else if (i.key == CMAKE_C_TOOLCHAIN_KEY)
            tcCPath = expandedValue;
        else if (i.key == CMAKE_CXX_TOOLCHAIN_KEY)
            tcCxxPath = expandedValue;
        else if (i.key == CMAKE_PREFIX_PATH_KEY)
            qtInstallDirs = CMakeConfigItem::cmakeSplitValue(expandedValue.toString());
    }

    QList<Task> result;
    // Validate Qt:
    if (qmakePath.isEmpty()) {
        if (version && version->isValid() && isQt4) {
            result << Task(Task::Warning, tr("CMake configuration has no path to qmake binary set, "
                                             "even though the kit has a valid Qt version."),
                           Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    } else {
        if (!version || !version->isValid()) {
            result << Task(Task::Warning, tr("CMake configuration has a path to a qmake binary set, "
                                             "even though the kit has no valid Qt version."),
                           Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        } else if (qmakePath != version->qmakeCommand() && isQt4) {
            result << Task(Task::Warning, tr("CMake configuration has a path to a qmake binary set "
                                             "that does not match the qmake binary path "
                                             "configured in the Qt version."),
                           Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }
    if (version && !qtInstallDirs.contains(version->qmakeProperty("QT_INSTALL_PREFIX")) && !isQt4) {
        if (version->isValid()) {
            result << Task(Task::Warning, tr("CMake configuration has no CMAKE_PREFIX_PATH set "
                                             "that points to the kit Qt version."),
                           Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }

    // Validate Toolchains:
    if (tcCPath.isEmpty()) {
        if (tcC && tcC->isValid()) {
            result << Task(Task::Warning, tr("CMake configuration has no path to a C compiler set, "
                                             "even though the kit has a valid tool chain."),
                           Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    } else {
        if (!tcC || !tcC->isValid()) {
            result << Task(Task::Warning, tr("CMake configuration has a path to a C compiler set, "
                                             "even though the kit has no valid tool chain."),
                           Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        } else if (tcCPath != tcC->compilerCommand()) {
            result << Task(Task::Warning, tr("CMake configuration has a path to a C compiler set "
                                             "that does not match the compiler path "
                                             "configured in the tool chain of the kit."),
                           Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }

    if (tcCxxPath.isEmpty()) {
        if (tcCxx && tcCxx->isValid()) {
            result << Task(Task::Warning, tr("CMake configuration has no path to a C++ compiler set, "
                                             "even though the kit has a valid tool chain."),
                           Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    } else {
        if (!tcCxx || !tcCxx->isValid()) {
            result << Task(Task::Warning, tr("CMake configuration has a path to a C++ compiler set, "
                                             "even though the kit has no valid tool chain."),
                           Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        } else if (tcCxxPath != tcCxx->compilerCommand()) {
            result << Task(Task::Warning, tr("CMake configuration has a path to a C++ compiler set "
                                             "that does not match the compiler path "
                                             "configured in the tool chain of the kit."),
                           Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }

    return result;
}

void CMakeConfigurationKitAspect::setup(Kit *k)
{
    if (k && !k->hasValue(CONFIGURATION_ID))
        k->setValue(CONFIGURATION_ID, defaultValue(k));
}

void CMakeConfigurationKitAspect::fix(Kit *k)
{
    Q_UNUSED(k);
}

KitAspect::ItemList CMakeConfigurationKitAspect::toUserOutput(const Kit *k) const
{
    const QStringList current = toStringList(k);
    return ItemList() << qMakePair(tr("CMake Configuration"), current.join(QLatin1String("<br>")));
}

KitAspectWidget *CMakeConfigurationKitAspect::createConfigWidget(Kit *k) const
{
    if (!k)
        return nullptr;
    return new Internal::CMakeConfigurationKitAspectWidget(k, this);
}

} // namespace CMakeProjectManager
