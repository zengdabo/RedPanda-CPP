/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "cppparser.h"
#include "parserutils.h"
#include "../utils.h"
#include "../qsynedit/highlighter/cpp.h"

#include <QApplication>
#include <QDate>
#include <QHash>
#include <QQueue>
#include <QThread>
#include <QTime>

static QAtomicInt cppParserCount(0);
CppParser::CppParser(QObject *parent) : QObject(parent),
    mMutex(QMutex::Recursive)
{
    mParserId = cppParserCount.fetchAndAddRelaxed(1);
    mSerialCount = 0;
    updateSerialId();
    mParsing = false;

    mIndex = clang_createIndex(0, 1);
    mLockCount = 0;

    mCppKeywords = CppKeywords;
    mCppTypeKeywords = CppTypeKeywords;
}

CppParser::~CppParser()
{
    while (true) {
        //wait for all methods finishes running
        {
            QMutexLocker locker(&mMutex);
            if (!mParsing && (mLockCount == 0)) {
              mParsing = true;
              break;
            }
        }
        QThread::msleep(50);
        QCoreApplication* app = QApplication::instance();
        app->processEvents();
    }
    clearIndex();
}

void CppParser::parseFile(const QString &fileName, bool inProject, bool onlyIfNotParsed, bool updateView)
{
    if (!mEnabled)
        return;
    {
        QMutexLocker locker(&mMutex);
        if (mParsing || mLockCount>0)
            return;
        updateSerialId();
        mParsing = true;
        if (updateView)
            emit onBusy();
        emit onStartParsing();
    }
    {
        auto action = finally([&,this]{
            mParsing = false;
            if (updateView)
                emit onEndParsing(1,1);
            else
                emit onEndParsing(0,0);
        });
        QString fName = fileName;
        if (onlyIfNotParsed && mTranslationUnits.contains(fileName))
            return;

        if (!mTranslationUnits.contains(fileName)) {
            CXTranslationUnit unit = mTranslationUnits.value(fileName);
            //todo:prepare unsavedFiles
            clang_reparseTranslationUnit(unit,0,nullptr,clang_defaultReparseOptions(unit));
        } else {
            CXTranslationUnit unit = clang_parseTranslationUnit(
                        mIndex,
                        fileName.toLocal8Bit().data(),
                        nullptr,
                        0,
                        nullptr,
                        0,
                        CXTranslationUnit_None);
            if (unit)
                mTranslationUnits.insert(fileName,unit);
            else
                return;
        }
    }
}

bool CppParser::freeze()
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return false;
    mLockCount++;
    return true;
}

bool CppParser::freeze(const QString &serialId)
{
    QMutexLocker locker(&mMutex);
    if (mParsing)
        return false;
    if (mSerialId!=serialId)
        return false;
    mLockCount++;
    return true;
}

QString CppParser::getHeaderFileName(const QString &relativeTo, const QString &line)
{
    QMutexLocker locker(&mMutex);
    return ::getHeaderFilename(relativeTo, line, mIncludePaths,
                               mProjectIncludePaths);
}

bool CppParser::isProjectHeaderFile(const QString &fileName)
{
    QMutexLocker locker(&mMutex);
    return ::isSystemHeaderFile(fileName,mProjectIncludePaths);
}

bool CppParser::isSystemHeaderFile(const QString &fileName)
{
    QMutexLocker locker(&mMutex);
    return ::isSystemHeaderFile(fileName,mIncludePaths);
}

void CppParser::parseFileList(bool updateView)
{
//    if (!mEnabled)
//        return;
//    {
//        QMutexLocker locker(&mMutex);
//        if (mParsing || mLockCount>0)
//            return;
//        updateSerialId();
//        mParsing = true;
//        if (updateView)
//            emit onBusy();
//        emit onStartParsing();
//    }
//    {
//        auto action = finally([&,this]{
//            mParsing = false;
//            if (updateView)
//                emit onEndParsing(mFilesScannedCount,1);
//            else
//                emit onEndParsing(mFilesScannedCount,0);
//        });
//        // Support stopping of parsing when files closes unexpectedly
//        mFilesScannedCount = 0;
//        mFilesToScanCount = mFilesToScan.count();
//        // parse header files in the first parse
//        foreach (const QString& file, mFilesToScan) {
//            if (isHFile(file)) {
//                mFilesScannedCount++;
//                emit onProgress(mCurrentFile,mFilesToScanCount,mFilesScannedCount);
//                if (!mPreprocessor.scannedFiles().contains(file)) {
//                    internalParse(file);
//                }
//            }
//        }
//        //we only parse CFile in the second parse
//        foreach (const QString& file,mFilesToScan) {
//            if (isCFile(file)) {
//                mFilesScannedCount++;
//                emit onProgress(mCurrentFile,mFilesToScanCount,mFilesScannedCount);
//                if (!mPreprocessor.scannedFiles().contains(file)) {
//                    internalParse(file);
//                }
//            }
//        }
//        mFilesToScan.clear();
//    }
}

bool CppParser::parsing() const
{
    return mParsing;
}

StatementKind CppParser::getStatementKindAt(const QString &filename, const int line, const int ch)
{
    CXTranslationUnit unit = mTranslationUnits.value(filename,nullptr);
    if (!unit)
        return StatementKind::skUnknown;
    CXFile file = clang_getFile(unit,filename.toLocal8Bit().data());
    CXSourceLocation sourceLocation = clang_getLocation(unit,file,line,ch);
    CXCursor cursor = clang_getCursor(unit,sourceLocation);
}

void CppParser::reset()
{
    while (true) {
        {
            QMutexLocker locker(&mMutex);
            if (!mParsing && mLockCount ==0) {
                mParsing = true;
                break;
            }
        }
        QThread::msleep(50);
        QCoreApplication* app = QApplication::instance();
        app->processEvents();
    }
    {
        mParsing=true;
        auto action = finally([this]{
            mParsing = false;
        });
        emit  onBusy();
        mIncludePaths.clear();
        mProjectIncludePaths.clear();
        mProjectFiles.clear();
        clearIndex();
        //recreate empty index
        mIndex = clang_createIndex(0,1);
    }
}

void CppParser::unFreeze()
{
    QMutexLocker locker(&mMutex);
    mLockCount--;
}

void CppParser::addIncludePaths(const QStringList &paths)
{
    QMutexLocker locker(&mMutex);
    foreach(const QString& s, paths) {
        mIncludePaths.insert(s);
    }
}

void CppParser::addProjectIncludePaths(const QStringList &paths)
{
    QMutexLocker locker(&mMutex);
    foreach(const QString& s, paths) {
        mProjectIncludePaths.insert(s);
    }
}

void CppParser::addProjectFiles(const QStringList &files)
{
    QMutexLocker locker(&mMutex);
    foreach(const QString& s, files) {
        mProjectFiles.insert(s);
    }
}


QString CppParser::prettyPrintStatement(const PStatement& statement, const QString& filename, int line)
{
    QString result;
    if (!statement->hintText.isEmpty()) {
      if (statement->kind != StatementKind::skPreprocessor)
          result = statement->hintText;
      else if (statement->command == "__FILE__")
          result = '"'+filename+'"';
      else if (statement->command == "__LINE__")
          result = QString("\"%1\"").arg(line);
      else if (statement->command == "__DATE__")
          result = QString("\"%1\"").arg(QDate::currentDate().toString(Qt::ISODate));
      else if (statement->command == "__TIME__")
          result = QString("\"%1\"").arg(QTime::currentTime().toString(Qt::ISODate));
      else
          result = statement->hintText;
    } else {
        switch(statement->kind) {
        case StatementKind::skFunction:
        case StatementKind::skVariable:
        case StatementKind::skParameter:
        case StatementKind::skClass:
            if (statement->scope!= StatementScope::ssLocal)
                result = getScopePrefix(statement)+ ' '; // public
            result += statement->type + ' '; // void
            result += statement->fullName; // A::B::C::Bar
            result += statement->args; // (int a)
            break;
        case StatementKind::skNamespace:
            result = statement->fullName; // Bar
            break;
        case StatementKind::skConstructor:
            result = getScopePrefix(statement); // public
            result += QObject::tr("constructor") + ' '; // constructor
            result += statement->type + ' '; // void
            result += statement->fullName; // A::B::C::Bar
            result += statement->args; // (int a)
            break;
        case StatementKind::skDestructor:
            result = getScopePrefix(statement); // public
            result += QObject::tr("destructor") + ' '; // constructor
            result += statement->type + ' '; // void
            result += statement->fullName; // A::B::C::Bar
            result += statement->args; // (int a)
            break;
        default:
            break;
        }
    }
    return result;
}

void CppParser::updateSerialId()
{
    mSerialId = QString("%1 %2").arg(mParserId).arg(mSerialCount);
}

int CppParser::parserId() const
{
    return mParserId;
}

void CppParser::setOnGetFileStream(const GetFileStreamCallBack &newOnGetFileStream)
{
    mOnGetFileStream = newOnGetFileStream;
}

void CppParser::clearIndex()
{
    foreach(CXTranslationUnit unit, mTranslationUnits.values()) {
        clang_disposeTranslationUnit(unit);
    }
    mTranslationUnits.clear();
    clang_disposeIndex(mIndex);
}

bool CppParser::enabled() const
{
    return mEnabled;
}

void CppParser::setEnabled(bool newEnabled)
{
    if (mEnabled!=newEnabled) {
        mEnabled = newEnabled;
        if (!mEnabled) {
            this->reset();
        }
    }
}

CppFileParserThread::CppFileParserThread(
        PCppParser parser,
        QString fileName,
        bool inProject,
        bool onlyIfNotParsed,
        bool updateView,
        QObject *parent):QThread(parent),
    mParser(parser),
    mFileName(fileName),
    mInProject(inProject),
    mOnlyIfNotParsed(onlyIfNotParsed),
    mUpdateView(updateView)
{

}

void CppFileParserThread::run()
{
    if (mParser && !mParser->parsing()) {
        mParser->parseFile(mFileName,mInProject,mOnlyIfNotParsed,mUpdateView);
    }
}

CppFileListParserThread::CppFileListParserThread(PCppParser parser,
                                                 bool updateView, QObject *parent):
    QThread(parent),
    mParser(parser),
    mUpdateView(updateView)
{

}

void CppFileListParserThread::run()
{
    if (mParser && !mParser->parsing()) {
        mParser->parseFileList(mUpdateView);
    }
}

void parseFile(PCppParser parser, const QString& fileName, bool inProject, bool onlyIfNotParsed, bool updateView)
{
    if (!parser)
        return;
    CppFileParserThread* thread = new CppFileParserThread(parser,fileName,inProject,onlyIfNotParsed,updateView);
    thread->connect(thread,
                    &QThread::finished,
                    thread,
                    &QThread::deleteLater);
    thread->start();
}

void parseFileList(PCppParser parser, bool updateView)
{
    if (!parser)
        return;
    CppFileListParserThread *thread = new CppFileListParserThread(parser,updateView);
    thread->connect(thread,
                    &QThread::finished,
                    thread,
                    &QThread::deleteLater);
    thread->start();
}
