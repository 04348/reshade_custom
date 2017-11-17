/**
 * Copyright (C) 2014 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/reshade#license
 */

#include "directory_watcher.hpp"
#include "string_codecvt.hpp"

#include <Windows.h>

namespace reshade::filesystem
{
	directory_watcher::directory_watcher(const path &path) :
		_path(path),
		_buffer(sizeof(FILE_NOTIFY_INFORMATION) + MAX_PATH * sizeof(WCHAR))
	{
		_file_handle = CreateFileW(path.wstring().c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
		_completion_handle = CreateIoCompletionPort(_file_handle, nullptr, reinterpret_cast<ULONG_PTR>(_file_handle), 1);

		OVERLAPPED overlapped = { };
		ReadDirectoryChangesW(_file_handle, _buffer.data(), static_cast<DWORD>(_buffer.size()), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, nullptr, &overlapped, nullptr);
	}
	directory_watcher::~directory_watcher()
	{
		CancelIo(_file_handle);

		CloseHandle(_file_handle);
		CloseHandle(_completion_handle);
	}

	bool directory_watcher::check(std::vector<path> &modifications)
	{
		DWORD transferred;
		ULONG_PTR key;
		OVERLAPPED *overlapped;

		if (!GetQueuedCompletionStatus(_completion_handle, &transferred, &key, &overlapped, 0))
		{
			return false;
		}

		static DWORD s_last_tick_count = 0;
		static std::string s_last_filename;

		auto record = reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(_buffer.data());
		const auto current_tick_count = GetTickCount();

		while (true)
		{
			const std::string filename = utf16_to_utf8(record->FileName, record->FileNameLength / sizeof(WCHAR));

			if (filename != s_last_filename || s_last_tick_count + 2000 < current_tick_count)
			{
				s_last_filename = filename;
				s_last_tick_count = current_tick_count;

				modifications.push_back(_path / filename);
			}

			if (record->NextEntryOffset == 0)
			{
				break;
			}

			record = reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(reinterpret_cast<const BYTE *>(record) + record->NextEntryOffset);
		}

		overlapped->hEvent = nullptr;

		ReadDirectoryChangesW(_file_handle, _buffer.data(), static_cast<DWORD>(_buffer.size()), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, nullptr, overlapped, nullptr);

		return true;
	}
}
