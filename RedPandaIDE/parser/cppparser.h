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
#ifndef CPPPARSER_H
#define CPPPARSER_H

#include <QMutex>
#include <QObject>
#include <QThread>
#include <QVector>
#include <clang-c/Index.h>
#include "statementmodel.h"
#include "cpptokenizer.h"
#include "cpppreprocessor.h"

class CppParser : public QObject
{
    Q_OBJECT

    using GetFileStreamCallBack = std::function<bool (const QString&, QStringList&)>;
public:
    explicit CppParser(QObject *parent = nullptr);
    ~CppParser();

    void parseFile(const QString& fileName, bool inProject,
                   bool onlyIfNotParsed = false, bool updateView = true);

    bool freeze();
    bool freeze(const QString& serialId);
    void unFreeze();

    void addIncludePaths(const QStringList &paths);
    void addProjectIncludePaths(const QStringList &paths);
    void addProjectFiles(const QStringList &files);
    QString getHeaderFileName(const QString& relativeTo, const QString& line);// both
    bool isProjectHeaderFile(const QString& fileName);
    bool isSystemHeaderFile(const QString &fileName);

    int parserId() const;

    bool enabled() const;
    void setEnabled(bool newEnabled);

    void reset();

    bool parsing() const;

    //class browser

    //code completion

    //rename symbol

    //goto definition/declaration

    //syntax coloring

    StatementKind  getStatementKindAt(const QString& filename, const int line, const int ch);

    //syntax checking

    void parseFileList(bool updateView = true);

    QString prettyPrintStatement(const PStatement& statement, const QString& filename, int line = -1);

    void setOnGetFileStream(const GetFileStreamCallBack &newOnGetFileStream);



signals:
    void onProgress(const QString& fileName, int total, int current);
    void onBusy();
    void onStartParsing();
    void onEndParsing(int total, int updateView);
private:
    void clearIndex();

    bool isIdentifier(const QString& token) const {
        return (!token.isEmpty() && isLetterChar(token.front())
                && !token.contains('\"'));
    }

    bool isLetterChar(const QChar& ch) const {
        return ch.isLetter() || ch == '_';
    }

    void updateSerialId();

private:
    int mParserId;
    int mSerialCount;
    QString mSerialId;
    bool mEnabled;

    CXIndex mIndex;

    QMap<QString,CXTranslationUnit> mTranslationUnits;

    QSet<QString> mIncludePaths;
    QSet<QString> mProjectIncludePaths;
    QSet<QString> mProjectFiles;

    //{ List of current project's file }
    //fMacroDefines : TList;
    int mLockCount; // lock(don't reparse) when we need to find statements in a batch
    bool mParsing;
    //fRemovedStatements: THashedStringList; //THashedStringList<String,PRemovedStatements>

    QMutex mMutex;
    GetFileStreamCallBack mOnGetFileStream;
    QMap<QString,SkipType> mCppKeywords;
    QSet<QString> mCppTypeKeywords;
};
using PCppParser = std::shared_ptr<CppParser>;

class CppFileParserThread : public QThread {
    Q_OBJECT
public:
    explicit CppFileParserThread(
            PCppParser parser,
            QString fileName,
            bool inProject,
            bool onlyIfNotParsed = false,
            bool updateView = true,
            QObject *parent = nullptr);

private:
    PCppParser mParser;
    QString mFileName;
    bool mInProject;
    bool mOnlyIfNotParsed;
    bool mUpdateView;

    // QThread interface
protected:
    void run() override;
};
using PCppParserThread = std::shared_ptr<CppFileParserThread>;

class CppFileListParserThread: public QThread {
    Q_OBJECT
public:
    explicit CppFileListParserThread(
            PCppParser parser,
            bool updateView = true,
            QObject *parent = nullptr);
private:
    PCppParser mParser;
    bool mUpdateView;
    // QThread interface
protected:
    void run() override;
};

void parseFile(
    PCppParser parser,
    const QString& fileName,
    bool inProject,
    bool onlyIfNotParsed = false,
    bool updateView = true);

void parseFileList(
        PCppParser parser,
        bool updateView = true);


#endif // CPPPARSER_H
