#!/bin/bash
set -e

echo "==> Building OpenKey (Release) with Developer ID..."
xcodebuild -project Sources/OpenKey/macOS/OpenKey.xcodeproj \
  -scheme OpenKey \
  -configuration Release \
  -derivedDataPath build \
  CODE_SIGN_IDENTITY="Developer ID Application: Vu Bach (JGXXBSPSUH)" \
  DEVELOPMENT_TEAM="JGXXBSPSUH" \
  build > /dev/null

echo "==> Re-signing with Hardened Runtime and Apple Timestamp..."
codesign --force --deep --options runtime --timestamp \
  --sign "Developer ID Application: Vu Bach (JGXXBSPSUH)" \
  build/Build/Products/Release/OpenKey.app

echo "==> Stopping running OpenKey..."
pkill -x OpenKey 2>/dev/null || true
sleep 0.5

echo "==> Deploying to /Applications/OpenKey.app..."
rm -rf /Applications/OpenKey.app
cp -R build/Build/Products/Release/OpenKey.app /Applications/OpenKey.app

echo "==> Unregistering Release build from LaunchServices..."
LSREG="/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister"
$LSREG -u build/Build/Products/Release/OpenKey.app 2>/dev/null || true

echo "==> Cleaning up build directory to prevent Spotlight duplicate..."
rm -rf build

echo "==> Launching updated OpenKey..."
open /Applications/OpenKey.app

echo "==> Done! OpenKey is now live at /Applications/OpenKey.app with zero duplicates."
