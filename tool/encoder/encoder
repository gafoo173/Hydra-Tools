import os
import sys
import marshal
import zlib
import base64
import hashlib
import random
import string
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
import inspect

class UniversalProtector:
    def __init__(self, key_mode='system', custom_key=None):
        """
        :param key_mode: 'system' (مشتق من النظام) أو 'universal' (مفتاح عشوائي يعمل على جميع الأجهزة)
        :param custom_key: مفتاح مخصص إذا كان key_mode هو 'custom'
        """
        self.anti_debug()
        self.key_mode = key_mode
        self.custom_key = custom_key
        self.aes_key = self._generate_key()
    
    def _generate_key(self):
        """إنشاء مفتاح التشفير حسب الوضع المحدد"""
        if self.key_mode == 'system':
            # مفتاح مشتق من خصائص النظام
            system_data = f"{sys.platform}{os.cpu_count()}{os.name}".encode()
            return hashlib.sha256(system_data).digest()
        elif self.key_mode == 'universal':
            # مفتاح عشوائي ثابت يعمل على جميع الأجهزة
            universal_seed = "fixed_seed_@12345!"
            return hashlib.sha256(universal_seed.encode()).digest()
        elif self.key_mode == 'custom' and self.custom_key:
            # استخدام مفتاح مخصص من المستخدم
            return hashlib.sha256(self.custom_key.encode()).digest()
        else:
            raise ValueError("وضع المفتاح غير صحيح")
    
    def anti_debug(self):
        """كشف وتجنب أدوات التصحيح"""
        debuggers = ['pydevd', 'pdb', 'debugpy', 'pydebug']
        for module in sys.modules:
            if any(debugger in module.lower() for debugger in debuggers):
                sys.exit(1)
    
    def protect_file(self, input_path, output_path):
        """تشفير الملف (بايثون أو BAT) مع الحفاظ على قابلية التنفيذ"""
        if input_path.endswith('.py'):
            self._protect_python_script(input_path, output_path)
        elif input_path.endswith('.bat'):
            self._protect_bat_file(input_path, output_path)
        else:
            raise ValueError("نوع الملف غير مدعوم. يجب أن يكون .py أو .bat")
    
    def _protect_python_script(self, script_path, output_path):
        """تشفير نص بايثون"""
        with open(script_path, 'r', encoding='utf-8') as f:
            code = f.read()
        
        encrypted_data = self._encrypt_layers(code)
        self._create_python_executable(encrypted_data, output_path, os.path.basename(script_path))
    
    def _protect_bat_file(self, bat_path, output_path):
        """تشفير ملف BAT مع الحفاظ على قابلية التنفيذ"""
        with open(bat_path, 'r', encoding='utf-8') as f:
            bat_content = f.read()
        
        # تشفير محتوى BAT
        encrypted_data = self._encrypt_layers(bat_content)
        
        # إنشاء ملف BAT جديد لفك التشفير والتنفيذ
        self._create_bat_executable(encrypted_data, output_path)
    
    def _encrypt_layers(self, data):
        """تطبيق طبقات التشفير المتعددة"""
        if isinstance(data, str):
            data = data.encode('utf-8')
        
        # 1. ضغط البيانات
        compressed = zlib.compress(data)
        
        # 2. تشفير AES
        iv = os.urandom(16)
        cipher = AES.new(self.aes_key, AES.MODE_CBC, iv)
        encrypted = cipher.encrypt(pad(compressed, AES.block_size))
        
        # 3. ترميز Base64
        return base64.b64encode(iv + encrypted).decode('utf-8')
    
    def _create_python_executable(self, encrypted_data, output_path, original_name):
        """إنشاء ملف بايثون تنفيذي"""
        template = f'''#!/usr/bin/env python3
# {original_name} - Protected Script

import os
import sys
import zlib
import base64
import hashlib
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad

def {self._obfuscate_name("get_key")}():
    """إنشاء المفتاح حسب الوضع المحدد"""
    if "{self.key_mode}" == "system":
        system_data = f"{{sys.platform}}{{os.cpu_count()}}{{os.name}}".encode()
        return hashlib.sha256(system_data).digest()
    elif "{self.key_mode}" == "universal":
        universal_seed = "fixed_seed_@12345!"
        return hashlib.sha256(universal_seed.encode()).digest()
    elif "{self.key_mode}" == "custom":
        return hashlib.sha256("{self.custom_key.encode().hex() if self.custom_key else ''}".encode()).digest()

def {self._obfuscate_name("decrypt")}(data):
    """فك تشفير البيانات"""
    key = {self._obfuscate_name("get_key")}()
    data = base64.b64decode(data.encode('utf-8'))
    iv, encrypted = data[:16], data[16:]
    cipher = AES.new(key, AES.MODE_CBC, iv)
    decrypted = unpad(cipher.decrypt(encrypted), AES.block_size)
    return zlib.decompress(decrypted)

ENCRYPTED_CODE = """
{encrypted_data}
"""

if __name__ == "__main__":
    try:
        decrypted = {self._obfuscate_name("decrypt")}(ENCRYPTED_CODE)
        exec(decrypted.decode('utf-8'))
    except Exception as e:
        print(f"[Error] Failed to execute: {{e}}")
        sys.exit(1)
'''
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(template)
        
        if sys.platform != 'win32':
            os.chmod(output_path, 0o755)
    
    def _create_bat_executable(self, encrypted_data, output_path):
        """إنشاء ملف BAT تنفيذي"""
        # إنشاء ملف BAT لفك التشفير وتنفيذ الأوامر الأصلية
        template = f'''@echo off
:: Protected BAT File - Auto Decrypting
setlocal enabledelayedexpansion

python -c "import base64,zlib,hashlib,os;from Crypto.Cipher import AES;from Crypto.Util.Padding import unpad;"
python -c "key = hashlib.sha256({'"fixed_seed_@12345!"' if self.key_mode == 'universal' else 'f\"{0}{1}{2}\".format(os.name,os.cpu_count(),sys.platform).encode()'}).digest();"
python -c "data = base64.b64decode(\"\"\"{encrypted_data}\"\"\");iv,encrypted=data[:16],data[16:];"
python -c "cipher = AES.new(key, AES.MODE_CBC, iv);decrypted=unpad(cipher.decrypt(encrypted),16);bat_content=zlib.decompress(decrypted).decode('utf-8');"
python -c "with open('temp_decrypted.bat','w') as f:f.write(bat_content);"
call temp_decrypted.bat
del temp_decrypted.bat
'''
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(template)
    
    def _obfuscate_name(self, name):
        """تعتيم أسماء الدوال"""
        return f'_{hashlib.md5(name.encode()).hexdigest()[:10]}'

def print_help():
    print("Universal Protector Tool - Advanced File Encryption")
    print("Usage:")
    print("  protector.py <input_file> <output_file> [key_mode] [custom_key]")
    print("\nKey Modes:")
    print("  system    - مفتاح مشتق من خصائص النظام (افتراضي)")
    print("  universal - مفتاح ثابت يعمل على جميع الأجهزة")
    print("  custom    - استخدام مفتاح مخصص")
    print("\nExamples:")
    print("  protector.py script.py protected_script.py system")
    print("  protector.py script.bat protected_script.bat universal")
    print("  protector.py script.py protected_script.py custom mysecretkey")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print_help()
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    key_mode = 'system'
    custom_key = None
    
    if len(sys.argv) > 3:
        key_mode = sys.argv[3].lower()
        if key_mode == 'custom' and len(sys.argv) > 4:
            custom_key = sys.argv[4]
    
    try:
        protector = UniversalProtector(key_mode=key_mode, custom_key=custom_key)
        protector.protect_file(input_file, output_file)
        print(f"File protected successfully: {output_file}")
        print(f"Key Mode: {key_mode}")
    except Exception as e:
        print(f"Error: {str(e)}")
        sys.exit(1)
