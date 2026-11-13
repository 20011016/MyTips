// Copyright (c) 2026. All Rights Reserved.

#include "SessionTextCodecLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogSessionTextCodec, Log, All);

namespace SessionTextCodec
{
	static const TCHAR HexDigitsUpper[] = TEXT("0123456789ABCDEF");
	static const TCHAR HexDigitsLower[] = TEXT("0123456789abcdef");

	/** Returns 0-15 for a hex digit, INDEX_NONE otherwise. */
	FORCEINLINE int32 HexCharToValue(const TCHAR Char)
	{
		if (Char >= TEXT('0') && Char <= TEXT('9'))
		{
			return static_cast<int32>(Char - TEXT('0'));
		}
		if (Char >= TEXT('A') && Char <= TEXT('F'))
		{
			return static_cast<int32>(Char - TEXT('A')) + 10;
		}
		if (Char >= TEXT('a') && Char <= TEXT('f'))
		{
			return static_cast<int32>(Char - TEXT('a')) + 10;
		}
		return INDEX_NONE;
	}

	/**
	 * Strict UTF-8 validation: rejects truncated sequences, overlong encodings,
	 * surrogate halves and code points above U+10FFFF.
	 */
	static bool IsValidUtf8(const uint8* Data, const int32 Num)
	{
		int32 Index = 0;
		while (Index < Num)
		{
			const uint8 Lead = Data[Index];
			if (Lead < 0x80)
			{
				++Index;
				continue;
			}

			int32 ExtraBytes = 0;
			uint32 Codepoint = 0;

			if ((Lead & 0xE0) == 0xC0)
			{
				ExtraBytes = 1;
				Codepoint = Lead & 0x1Fu;
			}
			else if ((Lead & 0xF0) == 0xE0)
			{
				ExtraBytes = 2;
				Codepoint = Lead & 0x0Fu;
			}
			else if ((Lead & 0xF8) == 0xF0)
			{
				ExtraBytes = 3;
				Codepoint = Lead & 0x07u;
			}
			else
			{
				// Continuation byte in lead position, or 0xF8-0xFF.
				return false;
			}

			if (Index + ExtraBytes >= Num)
			{
				return false;
			}

			for (int32 Offset = 1; Offset <= ExtraBytes; ++Offset)
			{
				const uint8 Trail = Data[Index + Offset];
				if ((Trail & 0xC0) != 0x80)
				{
					return false;
				}
				Codepoint = (Codepoint << 6) | (Trail & 0x3Fu);
			}

			static const uint32 MinCodepointPerLength[4] = { 0u, 0x80u, 0x800u, 0x10000u };
			if (Codepoint < MinCodepointPerLength[ExtraBytes] || Codepoint > 0x10FFFFu)
			{
				return false;
			}
			if (Codepoint >= 0xD800u && Codepoint <= 0xDFFFu)
			{
				return false;
			}

			Index += ExtraBytes + 1;
		}

		return true;
	}
}

FString USessionTextCodecLibrary::EncodeSessionText(const FString& InText, bool bUppercase)
{
	if (InText.IsEmpty())
	{
		return FString();
	}

	FTCHARToUTF8 Utf8(*InText);
	const uint8* Bytes = reinterpret_cast<const uint8*>(Utf8.Get());
	const int32 NumBytes = Utf8.Length();

	const TCHAR* Digits = bUppercase ? SessionTextCodec::HexDigitsUpper : SessionTextCodec::HexDigitsLower;

	FString Encoded;
	Encoded.Reserve(NumBytes * 2);
	for (int32 Index = 0; Index < NumBytes; ++Index)
	{
		Encoded.AppendChar(Digits[(Bytes[Index] >> 4) & 0x0F]);
		Encoded.AppendChar(Digits[Bytes[Index] & 0x0F]);
	}

	return Encoded;
}

FString USessionTextCodecLibrary::DecodeSessionText(const FString& InEncodedText)
{
	const int32 NumChars = InEncodedText.Len();
	if (NumChars == 0)
	{
		return FString();
	}

	// Not an even number of characters: cannot be hex, hand it back untouched.
	if ((NumChars & 1) != 0)
	{
		return InEncodedText;
	}

	TArray<uint8> Bytes;
	Bytes.SetNumUninitialized(NumChars / 2);

	for (int32 Index = 0; Index < NumChars; Index += 2)
	{
		const int32 High = SessionTextCodec::HexCharToValue(InEncodedText[Index]);
		const int32 Low = SessionTextCodec::HexCharToValue(InEncodedText[Index + 1]);
		if (High == INDEX_NONE || Low == INDEX_NONE)
		{
			// Plain text that happens to be even length, or a truncated payload.
			return InEncodedText;
		}
		Bytes[Index / 2] = static_cast<uint8>((High << 4) | Low);
	}

	if (!SessionTextCodec::IsValidUtf8(Bytes.GetData(), Bytes.Num()))
	{
		// Hex, but the bytes are not UTF-8 (for example a plain ASCII word made of hex letters).
		return InEncodedText;
	}

	FUTF8ToTCHAR Utf8(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
	return FString(Utf8.Length(), Utf8.Get());
}

int32 USessionTextCodecLibrary::GetEncodedLength(const FString& InText)
{
	if (InText.IsEmpty())
	{
		return 0;
	}

	FTCHARToUTF8 Utf8(*InText);
	return Utf8.Length() * 2;
}
