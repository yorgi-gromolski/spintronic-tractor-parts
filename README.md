# Unix Suriv Source Code

Once upon a time, I read an issue of a magazine, a real paper
magazine, named Mondo 2000. It had an article about computer
sesuriv, complete with stylin diagrams of disk files before
during and after infection.

About a month later, RTM unleashed the November, 1988 Internet worm.

I had to write a suriv. I had just bought an AT&T 3b1, a "UnixPC",
so I was ready to rock. This repo holds some of my code from that
time, which would be November 1988 to apparently October 1990.

I just looked through an old backup and found my code from that
period. As far as I remember, I've never put this code out anywhere
in the past.

## Portable Unix Suriv

I think that [unixpc](unixpc) documents the development of a
portable computer suriv, suitable for many of the Unix systems
of the day, BSD a.out and COFF executable file format,
and a number of filesystems.

* SunOS, M68k and SPARC
* UnixPC M68010 SysVR2, or R3
* SGI big-endian Mips R2000, EFS file system
* DECStation 2100, little-endian Mips R2000

I was a terrible coder back in those days. This is all K&R C,
and not very good K&R C at that. It's weird that the concept of
"portable" programs has disappeared from modern software engineering.

On the other hand, if you look at this code closely, you'll see
that I wrote functions that would directly read directory _files_,
and they were files back in those days. I had to figure out the
file format of various files systems: Sun's variant of BSD FFS,
SGI's EFS, I believe the original UFS on the UnixPC, and DEC's
BSD FFS variant.


I was also not big on version control back then. SCCS was somewhat
clunky, and RCS wasn't as widespread as it would become. CVS was still
well in the future. File names indicate a rough succession. Good luck.

Somewhat later, a book called [Unix Security: A practical tutorial](https://www.amazon.com/Unix-Security-Practical-Tutorial-McGraw-Hill/dp/0070025592) came out, featuring source code of a Unix suriv.
Some similarities exist between this suriv and the one in that book,
but I swear, I wrote this all on my own.

## Shell Script Suriv

Looks like [script](script) holds my attempts to write a `sh`
suriv based on Tom Duff's [1989 Computing Systems article](https://www.usenix.org/legacy/publications/compsystems/1989/spr_duff.pdf)
combined with self-reproducing programs.

## SunOS Sesuriv

I'm going to guess that [sunos.suriv.c](sunos.suriv.c)
was my first attempt to generalize the UnixPC suriv (Portable Unix Suriv$). I can't remember.

I guess that [sunos.appending](sunos.appending) directory has my
attempt at making a suriv that appends itself to the infected
file, rather than prepending. If memory serves, the distinction between
an "appending" and a "prepending" file infector was really big
in that time frame. Everyone, journalists, hackers, and the nascent
infosec community was trying to make a buck off this new technology
one way or the other.

Looks like I had to write position independent system call code
for this to work.

Oh my god, I loved SunOS back then.
