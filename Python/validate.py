import hashlib
import struct
import base64

SECRET_KEY = bytes([0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE])

def hexdump(buf, width=16):
    for i in range(0, len(buf), width):
        chunk = buf[i:i + width]
        hex_part = " ".join(f"{b:02X}" for b in chunk)
        print(f"{i:04X}: {hex_part}")

def generate_validation_key(data, size):
    digest = hashlib.blake2s(data, digest_size=size).digest()
    return base64.b64encode(digest)[:size+1]

def prompt_scores():
    try:
        salt = int(input('Enter salt: '), 16)
        trans = int(input('Translation Score: '))
        hint = int(input('Hint Score: '))
        logic = int(input('Logic Score: '))
        total = int(input('Total Score: '))
        sum_scores = trans + hint + logic
        if total != sum_scores:
            print(f'Invalid total score! Calculated total score: {sum_scores}')
            return prompt_scores()
        # Pack to match hash_data_t layout (STM32 ARM Cortex-M = little-endian)
        # B    = uint8_t  salt          (1 byte,  offset 0)
        # 8s   = uint8_t  secret_key[8] (8 bytes, offset 1)
        # 3x   = padding                (3 bytes, offset 9)  <- compiler aligns uint32_t to 4 bytes
        # 4I   = uint32_t x4 scores     (16 bytes, offset 12)
        return struct.pack('<B8s3x4I', salt, SECRET_KEY, trans, hint, logic, total)
    except ValueError as e:
        print('Invalid input, please enter numeric values!')
        return prompt_scores()


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    while True:
        data = prompt_scores()
        # hexdump(data)
        hash_data = generate_validation_key(data, 16)
        print(f'Validation Key: {hash_data.decode()}')
        yn = input('Do you wish to validate another one? [y/n]')
        if yn == 'n':
            break

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
