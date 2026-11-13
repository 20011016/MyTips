// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SessionTextCodecLibrary.generated.h"

/**
 * Helpers that make online session settings safe for non-ASCII text.
 *
 * Why this exists:
 *   The Steam online subsystem writes session settings to lobby data with TCHAR_TO_UTF8
 *   but reads them back with ANSI_TO_TCHAR (see SteamSessionKeys.h / OnlineSessionAsyncLobbySteam.cpp).
 *   Any character outside of 7-bit ASCII is therefore destroyed on the round trip, which breaks
 *   Chinese / Japanese / Korean room names, passwords and any other localized text.
 *
 *   Encoding the text to the hex string of its UTF-8 bytes sidesteps the problem entirely:
 *   hex digits are pure ASCII, so UTF-8 and ANSI produce identical bytes. Decoding restores
 *   the original string exactly.
 */
UCLASS()
class SESSIONTEXTCODEC_API USessionTextCodecLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Encodes a string into the hex representation of its UTF-8 bytes.
	 *
	 * Use the result as the VALUE of a session setting (Make Literal Session Property String).
	 * Unicode text of any language is supported (Chinese, Japanese, Korean, emoji, ...).
	 *
	 * Each UTF-8 byte becomes two characters, so the output is 2x the UTF-8 size:
	 *   "密码" -> 6 bytes -> "E5AF86E7A081" (12 characters)
	 *
	 * NOTE: The Steam backend in UE uses a 2000 byte buffer when reading lobby data, so keep the
	 * encoded result well under that (about 300 CJK characters). Use GetEncodedLength to check.
	 *
	 * @param InText		The text to encode. Empty input returns an empty string.
	 * @param bUppercase	Use upper case hex digits (A-F). Defaults to true.
	 * @return				ASCII-only hex string, or an empty string when InText is empty.
	 */
	UFUNCTION(BlueprintPure, Category = "Online|Session|Text Codec", meta = (DisplayName = "Encode Session Text (UTF8 to Hex)"))
	static FString EncodeSessionText(const FString& InText, bool bUppercase = true);

	/**
	 * Decodes a hex string produced by EncodeSessionText back into the original text.
	 *
	 * Safe to call on values that were never encoded:
	 *   - odd length, non-hex characters, or bytes that are not valid UTF-8
	 *     cause the input to be returned unchanged instead of producing garbage.
	 *
	 * @param InEncodedText	Hex string to decode.
	 * @return				The decoded text, or InEncodedText when it is not valid encoded text.
	 */
	UFUNCTION(BlueprintPure, Category = "Online|Session|Text Codec", meta = (DisplayName = "Decode Session Text (Hex to UTF8)"))
	static FString DecodeSessionText(const FString& InEncodedText);

	/**
	 * Length (in characters) that EncodeSessionText would produce for the given text.
	 * Handy for keeping a value under the backend buffer limit before creating a session.
	 *
	 * @param InText		Text that will be encoded.
	 * @return				Number of hex characters, i.e. UTF-8 byte count * 2.
	 */
	UFUNCTION(BlueprintPure, Category = "Online|Session|Text Codec", meta = (DisplayName = "Get Session Text Encoded Length"))
	static int32 GetEncodedLength(const FString& InText);
};
