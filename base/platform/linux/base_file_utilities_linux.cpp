// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_file_utilities_linux.h"

#include "base/platform/base_platform_file_utilities.h"
#include "base/platform/linux/base_linux_glib_helper.h"
#include "base/integration.h"
#include "base/algorithm.h"

#include <QtCore/QProcess>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtGui/QDesktopServices>

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusUnixFileDescriptor>
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>

namespace base::Platform {
namespace {

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
bool PortalShowInFolder(const QString &filepath) {
	const auto fd = open(QFile::encodeName(filepath).constData(), O_RDONLY);
	if (fd == -1) {
		return false;
	}

	auto message = QDBusMessage::createMethodCall(
		"org.freedesktop.portal.Desktop",
		"/org/freedesktop/portal/desktop",
		"org.freedesktop.portal.OpenURI",
		"OpenDirectory");

	message.setArguments({
		QString(),
		QVariant::fromValue(QDBusUnixFileDescriptor(fd)),
		QVariantMap()
	});

	close(fd);

	const QDBusError error = QDBusConnection::sessionBus().call(message);
	return !error.isValid();
}

bool DBusShowInFolder(const QString &filepath) {
	auto message = QDBusMessage::createMethodCall(
		"org.freedesktop.FileManager1",
		"/org/freedesktop/FileManager1",
		"org.freedesktop.FileManager1",
		"ShowItems");

	message.setArguments({
		QStringList{QUrl::fromLocalFile(filepath).toString()},
		QString()
	});

	const QDBusError error = QDBusConnection::sessionBus().call(message);
	return !error.isValid();
}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

bool ProcessShowInFolder(const QString &filepath) {
	auto fileManagerAppInfo = g_app_info_get_default_for_type(
		"inode/directory",
		false);

	if (!fileManagerAppInfo) {
		return false;
	}

	const auto fileManagerAppInfoId = QString(
		g_app_info_get_id(fileManagerAppInfo));

	g_object_unref(fileManagerAppInfo);

	if (fileManagerAppInfoId == qstr("dolphin.desktop")
		|| fileManagerAppInfoId == qstr("org.kde.dolphin.desktop")) {
		return QProcess::startDetached("dolphin", {
			"--select",
			filepath
		});
	} else if (fileManagerAppInfoId == qstr("nautilus.desktop")
		|| fileManagerAppInfoId == qstr("org.gnome.Nautilus.desktop")
		|| fileManagerAppInfoId == qstr("nautilus-folder-handler.desktop")) {
		return QProcess::startDetached("nautilus", {
			filepath
		});
	} else if (fileManagerAppInfoId == qstr("nemo.desktop")) {
		return QProcess::startDetached("nemo", {
			"--no-desktop",
			filepath
		});
	} else if (fileManagerAppInfoId == qstr("konqueror.desktop")
		|| fileManagerAppInfoId == qstr("kfmclient_dir.desktop")) {
		return QProcess::startDetached("konqueror", {
			"--select",
			filepath
		});
	}

	return false;
}

} // namespace

bool ShowInFolder(const QString &filepath) {
	const auto absolutePath = QFileInfo(filepath).absoluteFilePath();
	const auto absoluteDirPath = QFileInfo(filepath)
		.absoluteDir()
		.absolutePath();

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	if (DBusShowInFolder(absolutePath)) {
		return true;
	}

	if (PortalShowInFolder(absolutePath)) {
		return true;
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

	if (ProcessShowInFolder(absolutePath)) {
		return true;
	}

	if (g_app_info_launch_default_for_uri(
		g_filename_to_uri(
			absoluteDirPath.toUtf8().constData(),
			nullptr,
			nullptr),
		nullptr,
		nullptr)) {
		return true;
	}

	if (QDesktopServices::openUrl(QUrl::fromLocalFile(absoluteDirPath))) {
		return true;
	}

	return false;
}

QString CurrentExecutablePath(int argc, char *argv[]) {
	constexpr auto kMaxPath = 1024;
	char result[kMaxPath] = { 0 };
	auto count = readlink("/proc/self/exe", result, kMaxPath);
	if (count > 0) {
		auto filename = QFile::decodeName(result);
		auto deletedPostfix = qstr(" (deleted)");
		if (filename.endsWith(deletedPostfix)
			&& !QFileInfo(filename).exists()) {
			filename.chop(deletedPostfix.size());
		}
		return filename;
	}

	// Fallback to the first command line argument.
	return argc ? QFile::decodeName(argv[0]) : QString();
}

void RemoveQuarantine(const QString &path) {
}

// From http://stackoverflow.com/questions/2256945/removing-a-non-empty-directory-programmatically-in-c-or-c
bool DeleteDirectory(QString path) {
	if (path.endsWith('/')) {
		path.chop(1);
	}
	const auto pathRaw = QFile::encodeName(path);
	const auto d = opendir(pathRaw.constData());
	if (!d) {
		return false;
	}

	while (struct dirent *p = readdir(d)) {
		// Skip the names "." and ".." as we don't want to recurse on them.
		if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
			continue;
		}

		const auto fname = path + '/' + p->d_name;
		const auto encoded = QFile::encodeName(fname);
		struct stat statbuf;
		if (!stat(encoded.constData(), &statbuf)) {
			if (S_ISDIR(statbuf.st_mode)) {
				if (!DeleteDirectory(fname)) {
					closedir(d);
					return false;
				}
			} else {
				if (unlink(encoded.constData())) {
					closedir(d);
					return false;
				}
			}
		}
	}
	closedir(d);

	return !rmdir(pathRaw.constData());
}

bool RenameWithOverwrite(const QString &from, const QString &to) {
	const auto fromPath = QFile::encodeName(from);
	const auto toPath = QFile::encodeName(to);
	return (rename(fromPath.constData(), toPath.constData()) == 0);
}

void FlushFileData(QFile &file) {
	file.flush();
	if (const auto descriptor = file.handle()) {
		fsync(descriptor);
	}
}

bool IsNonExtensionMimeFrom(
		const QString &path,
		const flat_set<QString> &mimeTypes) {
	const auto utf8 = path.toUtf8();
	const auto file = gobject_wrap(g_file_new_for_path(utf8.constData()));
	if (!file) {
		return false;
	}
	const auto attributes = ""
		G_FILE_ATTRIBUTE_STANDARD_TYPE ","
		G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE ","
		G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE;
	const auto info = gobject_wrap(g_file_query_info(
		file.get(),
		attributes,
		G_FILE_QUERY_INFO_NONE,
		nullptr,
		nullptr));
	if (!info) {
		return false;
	}
	const auto type = g_file_info_get_content_type(info.get());
	if (!type) {
		Integration::Instance().logMessage(
			QString("Content-Type for path '%1' could not be guessed.")
				.arg(path));
		return false;
	}
	const auto utf16 = QString::fromUtf8(type).toLower();
	Integration::Instance().logMessage(
		QString("Content-Type for path '%1' guessed as '%2'.")
			.arg(path)
			.arg(utf16));
	return mimeTypes.contains(utf16);
}

} // namespace base::Platform
