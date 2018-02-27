/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cmakeprojectnodes.h"
#include "cmakeprojectconstants.h"
#include "cmakekitinformation.h"
#include "cmaketool.h"
#include "cmakeproject.h"

#include <coreplugin/fileiconprovider.h>
#include <cpptools/cpptoolsconstants.h>
#include <projectexplorer/target.h>
#include <utils/algorithm.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/optional.h>


#include <QClipboard>
#include <QDir>
#include <QGuiApplication>
#include <QMessageBox>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

namespace {
void noAutoAdditionNotify(const QStringList &filePaths, const ProjectExplorer::ProjectNode *node)
{
    Utils::optional<QString> srcPath{};

    for (const QString &file : filePaths) {
        if (Utils::mimeTypeForFile(file).name() == CppTools::Constants::CPP_SOURCE_MIMETYPE) {
            srcPath = file;
            break;
        }
    }

    if (srcPath) {
        QMessageBox::StandardButton reply =
            QMessageBox::question(nullptr, QMessageBox::tr("Copy to Clipboard?"),
                                  QMessageBox::tr("Files are not automatically added to the CMakeLists.txt file of the CMake project."
                                                  "\nCopy the path to the source files to the clipboard?"),
                                  QMessageBox::Yes | QMessageBox::No);

        if (QMessageBox::Yes == reply) {
            QClipboard *clip = QGuiApplication::clipboard();

            QDir projDir{node->filePath().toFileInfo().absoluteFilePath()};
            clip->setText(QDir::cleanPath(projDir.relativeFilePath(srcPath.value())));
        }
    }
}

}

CMakeInputsNode::CMakeInputsNode(const Utils::FileName &cmakeLists) :
    ProjectExplorer::ProjectNode(cmakeLists, generateId(cmakeLists))
{
    setPriority(Node::DefaultPriority - 10); // Bottom most!
    setDisplayName(QCoreApplication::translate("CMakeFilesProjectNode", "CMake Modules"));
    setIcon(QIcon(":/projectexplorer/images/session.png")); // TODO: Use a better icon!
    setListInProject(false);
}

QByteArray CMakeInputsNode::generateId(const Utils::FileName &inputFile)
{
    return inputFile.toString().toUtf8() + "/cmakeInputs";
}

bool CMakeInputsNode::showInSimpleTree() const
{
    return true;
}

CMakeListsNode::CMakeListsNode(const Utils::FileName &cmakeListPath) :
    ProjectExplorer::ProjectNode(cmakeListPath)
{
    static QIcon folderIcon = Core::FileIconProvider::directoryIcon(Constants::FILEOVERLAY_CMAKE);
    setIcon(folderIcon);
    setListInProject(false);
}

bool CMakeListsNode::showInSimpleTree() const
{
    return false;
}

bool CMakeListsNode::supportsAction(ProjectExplorer::ProjectAction action, const ProjectExplorer::Node *) const
{
    return action == ProjectExplorer::ProjectAction::AddNewFile;
}

Utils::optional<Utils::FileName> CMakeListsNode::visibleAfterAddFileAction() const
{
    Utils::FileName projFile{filePath()};
    return projFile.appendPath("CMakeLists.txt");
}

CMakeProjectNode::CMakeProjectNode(const Utils::FileName &directory, CMakeProject *project) :
    ProjectExplorer::ProjectNode(directory),
    m_project(project)
{
    setPriority(Node::DefaultProjectPriority + 1000);
    setIcon(QIcon(":/projectexplorer/images/projectexplorer.png")); // TODO: Use proper icon!
    setListInProject(false);
}

bool CMakeProjectNode::showInSimpleTree() const
{
    return true;
}

QString CMakeProjectNode::tooltip() const
{
    return QString();
}

bool CMakeProjectNode::supportsAction(ProjectExplorer::ProjectAction action, const Node *node) const
{
    Q_UNUSED(node);
    if (!m_project)
        return false;

    const auto t = m_project->activeTarget();
    if (!t)
        return false;

    switch (action) {
        case ProjectExplorer::AddNewFile:
        case ProjectExplorer::EraseFile:
        case ProjectExplorer::Rename:
            return true;
        default:
            break;
    }

    return false;
}

bool CMakeProjectNode::addFiles(const QStringList &filePaths, QStringList *)
{
    noAutoAdditionNotify(filePaths, this);
    return m_project ? m_project->addFiles(filePaths) : false;
}

bool CMakeProjectNode::deleteFiles(const QStringList &filePaths)
{
    return m_project ? m_project->eraseFiles(filePaths) : false;
}

bool CMakeProjectNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    return m_project ? m_project->renameFile(filePath, newFilePath) : false;
}

CMakeTargetNode::CMakeTargetNode(const Utils::FileName &directory, const QString &target) :
    ProjectExplorer::ProjectNode(directory, generateId(directory, target))
{
    setPriority(Node::DefaultProjectPriority + 900);
    setIcon(QIcon(":/projectexplorer/images/build.png")); // TODO: Use proper icon!
    setListInProject(false);
}

QByteArray CMakeTargetNode::generateId(const Utils::FileName &directory, const QString &target)
{
    return directory.toString().toUtf8() + "///::///" + target.toUtf8();
}

bool CMakeTargetNode::showInSimpleTree() const
{
    return true;
}

QString CMakeTargetNode::tooltip() const
{
    return m_tooltip;
}

bool CMakeTargetNode::supportsAction(ProjectExplorer::ProjectAction action,
                                     const ProjectExplorer::Node *) const
{
    return action == ProjectExplorer::ProjectAction::AddNewFile;
}

bool CMakeTargetNode::addFiles(const QStringList &filePaths, QStringList *)
{
    noAutoAdditionNotify(filePaths, this);
    return true; // Return always true as autoadd is not supported!
}

Utils::optional<Utils::FileName> CMakeTargetNode::visibleAfterAddFileAction() const
{
    Utils::FileName projFile{filePath()};
    return projFile.appendPath("CMakeLists.txt");
}

void CMakeTargetNode::setTargetInformation(const QList<Utils::FileName> &artifacts,
                                           const QString &type)
{
    m_tooltip = QCoreApplication::translate("CMakeTargetNode", "Target type: ") + type + "<br>";
    if (artifacts.isEmpty()) {
        m_tooltip += QCoreApplication::translate("CMakeTargetNode", "No build artifacts");
    } else {
        const QStringList tmp = Utils::transform(artifacts, &Utils::FileName::toUserOutput);
        m_tooltip += QCoreApplication::translate("CMakeTargetNode", "Build artifacts:") + "<br>"
                + tmp.join("<br>");
    }
}
