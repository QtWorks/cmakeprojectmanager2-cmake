CMakeProjectManager2
====================

Alternative CMake support for Qt Creator.

Main differents from original CMakeProject plugin:

* Project file list readed from file system instead of parsing .cbp (CodeBlocks) project file.
  It can be slow on big projects, but reloading initiating only during open project, run CMake,
  or user-initiated tree changes (add new files, erase files, rename file)
* You can add new files from Qt Creator. Note, you can create file outside project tree,
  but Qt Creator will display only files that present in project tree. Also, you should
  manualy describe this files in CMakeLists.txt.
* You can rename files in file system from Qt Creator now.
* You can erase files from files system from Qt Creator now.
* Toolchain file overriding via dialog by pointing cmake toolchain file.

Build plugin
------------

This plugin is oriented to latest Git version of Qt Creator, sorry I use it and I have no time
to support other stable versions.

### Prepare Qt Creator (without Full Build)

For example, all actions runs in directory `/tmp/qt-creator`

- Take full Qt Creator source tree from Git:
```bash
git clone git://gitorious.org/qt-creator/qt-creator.git qt-creator
```
- Create Qt Crator build tree:
```bash
mkdir qt-creator-build
cd qt-creator-build
```
- Create shadow build (I use Git-version of Qt for building, but you can use Qt from official site: simple change path to `qmake`):
```bash
/opt/qt-git/bin/qmake /tmp/qt-creator/qt-creator/qtcreator.pro
```
- *TODO: check, possible unneeded*. Don't start full build, but generating some files is requred (like ide_version.h):
  - For Qt5 based builds (this call fails but needed files it generate):
  ```bash
  make sub-src-clean
  ```
  - For Qt4 based builds:
  ```bash
  make src/Makefile
  make -C src plugins/Makefile
  make -C src/plugins coreplugin/Makefile
  ```

Ok, Qt Creator source and build tree is prepared. Note, I think, that GIT version of Qt Creator already installed, for example to /opt/qtcreator-git

### Build plugin
- Change directory to `/tmp/qt-creator`
- Take sources of CMakeProjectManager2 from repository
- Export two variables
  * **QTC_SOURCE** - point to Qt Creator source tree (in out case: `/tmp/qt-creator/qt-cretor`)
  * **QTC_BUILD**  - point to Qt Creator build tree (or installed files, like `/opt/qtcreator-git`). Use this variable only in case, when you want
                     to put compiled files directly to the QtC tree. It fails, if QtC owned by other user but run build with sudo is a very very
                     bad idea.
  * **QTC_BUILD_INPLACE** - undefined by default. In most cases must be same to the **QTC_BUILD**. If defined, this location will be used as output
                            location for compiled plugin files (.so). As a result, you can omit install step. Useful only for individual users QtC 
                            installation at the home directory. If QtC installation does not owned by user who invoke compilation linking will be
                            fails. Strongly recommends do not use this variable, but use QTC_PREFIX for qmake and INSTALL_ROOT for make install.
  ```bash
  export QTC_SOURCE=/tmp/qt-creator/qt-creator
  export QTC_BUILD=/opt/qtcreator-git
  ```
- Create directory for shadow build:
```bash
mkdir cmakeprojectmanager2-build
cd cmakeprojectmanager2-build
```
- Configure plugin:
```bash
/opt/qt-git/bin/qmake QTC_PREFIX=/opt/qtcreator-git
                      LIBS+=-L${QTC_BUILD}/lib/qtcreator \
                      LIBS+=-L${QTC_BUILD}/lib/qtcreator/plugins \
                      ../cmakeprojectmanager2-git/cmakeprojectmanager.pro
```
  LIBS need for linker. QTC_PREFIX by default is `/usr/local`.
- Build:
```bash
 make -j8
 ```
- Install
  - to QTC_PREFIX:
  ```bash
  sudo make install
  ```
  - for packagers:
  ```bash
  make INSTALL_ROOT=/tmp/plugin install
  ```

Restart Qt Creator, go to Help -> About plugins and turn off default CMakeProjectManager plugin.

### Build plugin from Qt Creator
- Open `cmakeprojectmanager.pro`, go to Projects layout (see left panel)
- Look to Build Steps section and add next params for qmake:
```bash
LIBS+=-L${QTC_BUILD}/lib/qtcreator
LIBS+=-L${QTC_BUILD}/lib/qtcreator/plugins
```
- Look to Build Environment and define next variables:
```bash
QTC_SOURCE=/tmp/qt-creator/qt-creator
QTC_BUILD=/opt/qtcreator-git
```

Now you can build plugin from Qt Creator.

Prebuilt binaries
-----------------

Now Ubuntu 14.04/16.04 / Mint 17.x/18.x PPA with master-git Qt Creator and CMakePrjectManager2 plugin:<br />
https://launchpad.net/~adrozdoff/+archive/ubuntu/qtcreator-git

This repo depdens on next Qt repository: **ppa:beineri/opt-qt561-trusty**

To install next steps is required (Trusty):
```bash
sudo apt-add-repository ppa:adrozdoff/llvm-backport (i386)
sudo apt-add-repository ppa:adrozdoff/llvm-backport-x64 (x86_64)
sudo apt-add-repository ppa:beineri/opt-qt561-trusty
sudo apt-add-repository ppa:adrozdoff/qtcreator-git
sudo apt-get update
sudo apt-get install qtcreator-git qtcreator-git-plugin-cmake2
```

Xenial:
```bash
sudo apt-add-repository ppa:beineri/opt-qt561-xenial
sudo apt-add-repository ppa:adrozdoff/qtcreator-git
sudo apt-get update
sudo apt-get install qtcreator-git qtcreator-git-plugin-cmake2
```

TODO & Roadmap
--------------

Actual tasks and todo can be looks at the Issue page: https://github.com/h4tr3d/cmakeprojectmanager2/issues


Sync with Qt Creator upstream plugin
------------------------------------

1. Update Qt Creator git repository, moves to the master branch
2. Create patches for new changes
```
git format-patch <REVISION_SINCE> -- src/plugins/cmakeprojectmanager
```
REVISION_SINCE can be found via 'git log' by comments or Change-Id.
3. Go to CMakeProjectManager2 source tree and change branch to the `qtc-master`
4. Copy patches from the step 2 to the CMakeProjectManager2 source tree root
5. Apply patches:
```
git am -p4 *.patch
```
6. Change branch to the `master`, merge new changes and resolve conflicts
