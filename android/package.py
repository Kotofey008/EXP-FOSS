#!/usr/bin/env python3
"""Package the built native application. Requires aapt, framework resources, zipalign and JDK."""
import os
import pathlib
import subprocess
import zipfile
root = pathlib.Path(__file__).resolve().parents[1]
out = root / "dist"
out.mkdir(exist_ok=True)
aapt = os.environ.get("AAPT", "aapt")
framework = os.environ["ANDROID_FRAMEWORK"]
zipalign = os.environ.get("ZIPALIGN", "zipalign")
unsigned = out / "exploration-unsigned.apk"
aligned = out / "exploration-aligned.apk"
final = out / "exploration-debug.apk"
subprocess.run([aapt, "package", "-f", "-M", str(root / "android/AndroidManifest.xml"), "-A", str(root / "android/assets"), "-I", framework, "-F", str(unsigned)], check=True)
with zipfile.ZipFile(unsigned, "a", compression=zipfile.ZIP_STORED) as archive:
    archive.write(root / "build-android-x86/libexploration_app.so", "lib/x86/libexploration_app.so")
subprocess.run([zipalign, "-f", "4", str(unsigned), str(aligned)], check=True)
keystore = out / "debug.p12"
if not keystore.exists():
    subprocess.run(["keytool", "-genkeypair", "-keystore", str(keystore), "-storepass", "android", "-keypass", "android", "-alias", "androiddebugkey", "-dname", "CN=Exploration Reconstruction Debug", "-keyalg", "RSA", "-keysize", "2048", "-validity", "3650"], check=True)
signer = os.environ["APKSIGNER_JAR"]
subprocess.run(["java", "-jar", signer, "sign", "--ks", str(keystore), "--ks-pass", "pass:android", "--key-pass", "pass:android", "--out", str(final), str(aligned)], check=True)
subprocess.run(["java", "-jar", signer, "verify", "--verbose", str(final)], check=True)
subprocess.run([zipalign, "-c", "4", str(final)], check=True)
print(final)
