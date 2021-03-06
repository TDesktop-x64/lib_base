// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base::Platform {

bool CloseProcesses(const QString &filename);

inline bool IsNonExtensionMimeFrom(
        const QString &path,
        const flat_set<QString> &mimeTypes) {
    return false;
}

} // namespace base::Platform
