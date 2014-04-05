#!/usr/bin/env python

import frame_pb2 as pb
import struct
import getopt
import sys
import IPython

def getFrameLength(f):
        return struct.unpack("Q", f.read(8))[0]

def getFrame(f):
        return pb.frame.FromString(f.read(getFrameLength(f)))

#bitsize to format
btf = {8 : 'b', 32 : 'I'}

def printOperandList(l):
        #IPython.embed()
        res = ""
        regs = filter(lambda x : x.operand_info_specific.ListFields()[0][0].name == "reg_operand", l)
        mems = filter(lambda x : x.operand_info_specific.ListFields()[0][0].name == "mem_operand", l)
        for o in regs:
                v = struct.unpack("I", o.value)[0]
                res += "\treg: %s, value: 0x%08lx\n" % (o.operand_info_specific.reg_operand.name, v)
        res.strip()
        for o in mems:
                v = struct.unpack(btf[o.bit_length], o.value)[0]
                res += "\tmem: 0x%08lx, value: 0x%08lx\n" % (o.operand_info_specific.mem_operand.address, v)
        res.strip()
        return res

def skipFrames(infile, cnt):
        fr = None
        for x in range(0, cnt):
            fr = getFrame(infile)
        return fr

def gotoFrame(infile, cnt):
        infile.seek(0x30)
        return skipFrames(infile, cnt)

def gotoAddress(infile, addr, debug=False):
        infile.seek(0x30)
        fr = getFrame(infile)
        cnt = 1
        while fr.std_frame.address != addr:
                fr = getFrame(infile)
                cnt += 1
        if debug and fr.std_frame.address == addr:
                print "Frame # %i is at addr: 0x%08lx" % (cnt, addr)
        return fr

def printFrame(f):
        print "PRE: %s" % printOperandList(f.std_frame.operand_pre_list.elem)
        print "POST: %s" % printOperandList(f.std_frame.operand_post_list.elem)

def process(infileName, outfileName=None, maxCnt=0):
        out = sys.stdout
        if outfileName:
                out = open(outfileName, 'w')
        infile = open(infileName)

        (infile, metaMaxCnt) = getMetaData(infile)

        if maxCnt == 0:
                maxCnt = metaMaxCnt

        cnt = 0

        print "maxCnt: %i" % maxCnt

        while (cnt <= maxCnt):
                cnt += 1
                try:
                    fr = getFrame(infile)
                except google.protobuf.message.DecodeError, e:
                    print "maxCnt: %i, cnt: %i\n" % (maxCnt, cnt)
                    print e
                    break
                out.write("0x%x, %r\n" % (fr.std_frame.address, fr.std_frame.rawbytes))

def getMetaData(f):
        f.seek(0x20)
        numFrames = struct.unpack("Q", f.read(8))[0] - 1
        f.seek(0x30) #move to first frame
        return(f, numFrames)
        
def main():
        debug = 0
        maxCnt = 0
        infile = None
        outfile = None
        opts,argv = getopt.getopt(sys.argv[1:], "f:c:o:d") 
        for k,v in opts:
                if k == '-d':
                        debug += 1
                if k == '-f':
                        infile = v
                if k == '-o':
                        outfile = v
                if k == '-c':
                        maxCnt = int(v)

        if infile:
                process(infile, outfile, maxCnt)

if __name__ == "__main__":
        main()
