#include "KeyStorageFactory.h"

#include "KAA/include/exception/operation_failure.h"

#include "CRC32BasedKeyStorage.h"
#include "MD5BasedKeyStorage.h"

namespace KAA
{
	namespace FileSecurity
	{
		std::auto_ptr<KeyStorage> CreateKeyStorage(const key_storage_t type, filesystem::path::directory path)
		{
			switch(type)
			{
			case md5_based:
				return std::auto_ptr<KeyStorage> { new MD5BasedKeyStorage { std::move(path) } };
			case crc32_based:
				return std::auto_ptr<KeyStorage> { new CRC32BasedKeyStorage { std::move(path) } };
			default:
				{
					const std::wstring source { __FUNCTIONW__ };
					const std::wstring description { L"Unable to create a specified key storage class instance." };
					const KAA::operation_failure::status_code_t reason = KAA::operation_failure::R_INVALID_ARGUMENT;
					const KAA::operation_failure::severity_t severity = KAA::operation_failure::S_ERROR;
					throw operation_failure(source, description, reason, severity);
				}
			}
		}
	}
}
