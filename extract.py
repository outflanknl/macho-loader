import argparse

from macholib.MachO import MachO
from macholib.mach_o import LC_SEGMENT_64, LC_SEGMENT


def extract_macho(macho_path):
    macho = MachO(macho_path)
    for header in macho.headers:
        for load_cmd, cmd, data in header.commands:
            if load_cmd.cmd in (LC_SEGMENT_64, LC_SEGMENT):
                if cmd.segname.decode("utf-8").strip("\x00") == "__TEXT":
                    for section in data:
                        if section.sectname.decode("utf-8").strip("\x00") == "__text":
                            with open(macho_path, "rb") as f:
                                f.seek(section.offset)
                                data = f.read(section.size)
                                print("[+] Success: Extracted loader")
                                return data

    raise Exception("[-] Error: Loader not found")


def create_shellcode(args):
    loader = extract_macho(args.loader)
    with open(args.implant, "rb") as f:
        implant = f.read()

    shellcode = loader + implant
    print(f"Created {len(shellcode)} byte shellcode")

    with open(args.output, "wb") as f:
        f.write(shellcode)


def main():
    parser = argparse.ArgumentParser(description="")
    parser.add_argument("loader", help="Path to the reflective loader dylib")
    parser.add_argument("implant", help="Path to the implant dylib")
    parser.add_argument("output", help="Path to write the output file")
    args = parser.parse_args()

    create_shellcode(args)


if __name__ == "__main__":
    main()
