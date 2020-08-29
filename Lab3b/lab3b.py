#!/usr/bin/python

import sys
debug = 0;

def main():
    if len(sys.argv) <= 1:
        sys.stderr.write('Needs a file summary csv-file as argument\n')
        exit(1)
    try:
        file_summary = open(sys.argv[1], "r")
    except:
        sys.stderr.write('Error opening file\n')
        exit(1)

    if len(sys.argv) == 3 and sys.argv[2] == 'debug':
        debug = 1

    lines = file_summary.readlines()
    for line in lines:
        fields = line.split(",")
        output_type = fields[0]
        
        if output_type == 'SUPERBLOCK':
            blocks_count = fields[1]
            inode_count = fields[2]
            if debug:
                print("Blocks_count: {}, Inode_count: {}".format(blocks_count, inode_count))

if __name__ == "__main__":
    main()
