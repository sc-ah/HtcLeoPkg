#include <PiDxe.h>

#include <Library/LKEnvLib.h>

#include <Library/KeypadDeviceHelperLib.h>
#include <Library/KeypadDeviceImplLib.h>
#include <Protocol/KeypadDevice.h>

typedef enum {
  KEY_DEVICE_TYPE_UNKNOWN,
  KEY_DEVICE_TYPE_LEGACY,
  KEY_DEVICE_TYPE_KEYMATRIX,
  KEY_DEVICE_TYPE_TLMM,
  KEY_DEVICE_TYPE_PM8X41,
  KEY_DEVICE_TYPE_PM8X41_PON,
} KEY_DEVICE_TYPE;

typedef struct {
  KEY_CONTEXT     EfiKeyContext;
  BOOLEAN         IsValid;
  KEY_DEVICE_TYPE DeviceType;

  // gpio
  UINT8   Gpio;
  BOOLEAN ActiveLow;

  // gpio keymatrix

  UINT8 GpioIn;
  UINT8 GpioOut;

  // pon
  UINT32 PonType;
} KEY_CONTEXT_PRIVATE;

STATIC KEY_CONTEXT_PRIVATE KeyContextPower;
STATIC KEY_CONTEXT_PRIVATE KeyContextVolumeUp;
STATIC KEY_CONTEXT_PRIVATE KeyContextVolumeDown;
STATIC KEY_CONTEXT_PRIVATE KeyContextVolumeBack;
STATIC KEY_CONTEXT_PRIVATE KeyContextVolumeWindows;
STATIC KEY_CONTEXT_PRIVATE KeyContextVolumeHome;
STATIC KEY_CONTEXT_PRIVATE KeyContextVolumeDial;

STATIC KEY_CONTEXT_PRIVATE *KeyList[] = {
    &KeyContextPower,      &KeyContextVolumeUp,      &KeyContextVolumeDown,
    &KeyContextVolumeBack, &KeyContextVolumeWindows, &KeyContextVolumeHome,
    &KeyContextVolumeDial};

STATIC
VOID KeypadInitializeKeyContextPrivate(KEY_CONTEXT_PRIVATE *Context)
{
  Context->IsValid    = FALSE;
  Context->Gpio       = 0;
  Context->DeviceType = KEY_DEVICE_TYPE_UNKNOWN;
  Context->ActiveLow  = FALSE;
}

STATIC
KEY_CONTEXT_PRIVATE *KeypadKeyCodeToKeyContext(UINT32 KeyCode)
{
  if (KeyCode == 114)
    return &KeyContextVolumeDown;
  else if (KeyCode == 115)
    return &KeyContextVolumeUp;
  else if (KeyCode == 116)
    return &KeyContextPower;
  else if (KeyCode == 116)
    return &KeyContextBack;
  else if (KeyCode == 117)
    return &KeyContextWindows;
  else if (KeyCode == 118)
    return &KeyContextHome;
  else if (KeyCode == 119)
    return &KeyContextDial;
  else
    return NULL;
}

RETURN_STATUS
EFIAPI
KeypadDeviceImplConstructor(VOID)
{
  UINTN                Index;
  KEY_CONTEXT_PRIVATE *StaticContext;

  // Reset all keys
  for (Index = 0; Index < ARRAY_SIZE(KeyList); Index++) {
    KeypadInitializeKeyContextPrivate(KeyList[Index]);
  }

  // Configure keys

  // power button
  StaticContext             = KeypadKeyCodeToKeyContext(116);
  StaticContext->DeviceType = KEY_DEVICE_TYPE_LEGACY;
  StaticContext->Gpio       = 94;
  StaticContext->ActiveLow  = 0x1 & 0x1;
  StaticContext->IsValid    = TRUE;

  // back button
  StaticContext             = KeypadKeyCodeToKeyContext(116);
  StaticContext->DeviceType = KEY_DEVICE_TYPE_KEYMATRIX;
  StaticContext->GpioOut    = 33;
  StaticContext->GpioIn     = 42;
  StaticContext->ActiveLow  = 0x1 & 0x1;
  StaticContext->IsValid    = TRUE;

  // power button button again replace this with the rest of the keymap
  // StaticContext             = KeypadKeyCodeToKeyContext(116);
  // StaticContext->DeviceType = KEY_DEVICE_TYPE_LEGACY;
  // StaticContext->Gpio       = 94;
  // StaticContext->ActiveLow  = 0x1 & 0x1;
  // StaticContext->IsValid    = TRUE;

  return RETURN_SUCCESS;
}

EFI_STATUS EFIAPI KeypadDeviceImplReset(KEYPAD_DEVICE_PROTOCOL *This)
{
  LibKeyInitializeKeyContext(&KeyContextPower.EfiKeyContext);
  KeyContextPower.EfiKeyContext.KeyData.Key.UnicodeChar = CHAR_CARRIAGE_RETURN;

  LibKeyInitializeKeyContext(&KeyContextVolumeUp.EfiKeyContext);
  KeyContextVolumeUp.EfiKeyContext.KeyData.Key.ScanCode = SCAN_UP;

  LibKeyInitializeKeyContext(&KeyContextVolumeDown.EfiKeyContext);
  KeyContextVolumeDown.EfiKeyContext.KeyData.Key.ScanCode = SCAN_DOWN;

  // ToDo: replace with different keys that are returned

  LibKeyInitializeKeyContext(&KeyContextBack.EfiKeyContext);
  KeyContextBack.EfiKeyContext.KeyData.Key.ScanCode = SCAN_DOWN;

  LibKeyInitializeKeyContext(&KeyContextWindows.EfiKeyContext);
  KeyContextWindows.EfiKeyContext.KeyData.Key.ScanCode = SCAN_DOWN;

  LibKeyInitializeKeyContext(&KeyContextHome.EfiKeyContext);
  KeyContextHome.EfiKeyContext.KeyData.Key.ScanCode = SCAN_DOWN;

  LibKeyInitializeKeyContext(&KeyContextDial.EfiKeyContext);
  KeyContextDial.EfiKeyContext.KeyData.Key.ScanCode = SCAN_DOWN;

  // Todo: end

  return EFI_SUCCESS;
}

extern void gpio_set(unsigned n, unsigned on);
extern int  gpio_get(unsigned n);

EFI_STATUS KeypadDeviceImplGetKeys(
    KEYPAD_DEVICE_PROTOCOL *This, KEYPAD_RETURN_API *KeypadReturnApi,
    UINT64 Delta)
{
  UINT8   GpioStatus;
  BOOLEAN IsPressed;
  INTN    RC;
  UINTN   Index;
  DEBUG((EFI_D_ERROR, "KeypadDeviceImplGetKeys!\n"));

  for (Index = 0; Index < ARRAY_SIZE(KeyList); Index++) {
    KEY_CONTEXT_PRIVATE *Context = KeyList[Index];

    // check if this is a valid key
    if (Context->IsValid == FALSE)
      continue;

    // get status
    if (Context->DeviceType == KEY_DEVICE_TYPE_TLMM) {
      // GpioStatus = gGpioTlmm->Get(Context->Gpio);
      RC = 0;
    }
    else if (Context->DeviceType == KEY_DEVICE_TYPE_PM8X41) {
      // RC = gPm8x41->pm8x41_gpio_get(Context->Gpio, &GpioStatus);
    }
    else if (Context->DeviceType == KEY_DEVICE_TYPE_PM8X41_PON) {
      if (Context->PonType == 0x1) {
        //  GpioStatus = gPm8x41->pm8x41_resin_status();
      }
      else if (Context->PonType == 0x0) {
        //  GpioStatus = gPm8x41->pm8x41_get_pwrkey_is_pressed();
      }
      else
        continue;

      RC = 0;
    }
    else if (Context->DeviceType == KEY_DEVICE_TYPE_LEGACY) {
      // impliement hd2 gpio shit here
      GpioStatus = gpio_get(Context->Gpio);
      DEBUG((EFI_D_ERROR, "LEGACY KEYTYPE GPIO STATUS: %d!\n", GpioStatus));
      gpio_set(48, 1);
      RC = -1;
    }
    else if (Context->DeviceType == KEY_DEVICE_TYPE_KEYMATRIX) {
      DEBUG((EFI_D_ERROR, "KEYMATRIX!\n"));
      gpio_set(Context->GpioOut, 0);
      GpioStatus = gpio_get(Context->GpioIn);
      RC         = -1;
    }
    else {
      continue;
    }
    if (RC != 0)
      continue;

    // update key status
    // 0000 ^0001 = 0001 = decimal 1
    IsPressed = (GpioStatus ? 1 : 0) ^ Context->ActiveLow;

    if (IsPressed) {
      // impl a timed callback here that enables gpio 48 for a few secs and then
      // the callback function should disable it again
    }

    if (Context->DeviceType == KEY_DEVICE_TYPE_KEYMATRIX) {
      gpio_set(Context->GpioOut, 1);
    }

    LibKeyUpdateKeyStatus(
        &Context->EfiKeyContext, KeypadReturnApi, IsPressed, Delta);
  }

  return EFI_SUCCESS;
}
