#ifndef SYSTEMCONSTS_H
#define SYSTEMCONSTS_H

#include <QStringList>

#define APP_SETTSINGS_FILENAME "redpandacpp.ini"
#define GCC_PROGRAM     "gcc.exe"
#define GPP_PROGRAM     "g++.exe"
#define GDB_PROGRAM     "gdb.exe"
#define GDB32_PROGRAM   "gdb32.exe"
#define MAKE_PROGRAM    "mingw32-make.exe"
#define WINDRES_PROGRAM "windres.exe"
#define GPROF_PROGRAM   "gprof.exe"
#define CLEAN_PROGRAM   "rm.exe"
#define CPP_PROGRAM     "cpp.exe"

#define NULL_FILE       "NUL"

class SystemConsts
{
public:
    SystemConsts();
    const QStringList& defaultFileFilters() const noexcept;
    const QString& defaultFileFilter() const noexcept;
    void addDefaultFileFilter(const QString& name, const QString& fileExtensions);
private:
    QStringList mDefaultFileFilters;
};

extern SystemConsts* pSystemConsts;
#endif // SYSTEMCONSTS_H
