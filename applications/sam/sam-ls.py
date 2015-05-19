#!/usr/bin/env python
# PYTHON_ARGCOMPLETE_OK

import os
from datetime import date
import argparse

# python modules to easily get completion, colors, indent text...
import argcomplete
from clint.textui import colored, puts, indent

# parser of sequence
from pySequenceParser import sequenceParser

# sam common functions
import common


def printItem(item, args, level):
    """
    Print the item depending on the command line options.
    """
    itemType = item.getType()

    filePath = ''
    detailed = ''
    detailedSequence = ''

    # sam-ls -l
    if args.longListing:
        # type - date - size
        characterFromType = 'a'
        if itemType == sequenceParser.eTypeUndefined:
            characterFromType = '?'
        elif itemType == sequenceParser.eTypeFolder:
            characterFromType = 'd'
        elif itemType == sequenceParser.eTypeFile:
            characterFromType = 'f'
        elif itemType == sequenceParser.eTypeSequence:
            characterFromType = 's'
            # [ begin : end ] nbFiles - nbMissingFiles
            sequence = item.getSequence()
            detailedSequence += '[' + str(sequence.getFirstTime()) + ':' + str(sequence.getLastTime()) + '] '
            detailedSequence += str(sequence.getNbFiles()) + ' files'
            detailedSequence += ( ', ' + str(sequence.getNbMissingFiles() ) + ' missing files') if sequence.hasMissingFile() else ''
            detailedSequence += ' \t'
        elif itemType == sequenceParser.eTypeLink:
            characterFromType = 'l'
        itemStat = sequenceParser.ItemStat(item)
        lastUpdate = date.fromtimestamp(itemStat.modificationTime).strftime('%d/%m/%y')
        detailed += characterFromType + '  ' + lastUpdate + '  ' + str(itemStat.size)
        detailed += ' \t'

    # sam-ls --absolute-path
    if args.absolutePath:
        filePath += os.path.abspath(item.getFolder()) + '/'

    # sam-ls --relative-path
    if args.relativePath:
        filePath += (item.getFolder() if item.getFolder()[0] != '/' else '.')
        filePath += ('/' if filePath[-1] != '/' else '')

    # filename
    if args.color:
        if itemType == sequenceParser.eTypeUndefined:
            filePath = colored.red(os.path.join(filePath, item.getFilename()))
        elif itemType == sequenceParser.eTypeFolder:
            filePath = colored.blue(os.path.join(filePath + item.getFilename()), bold=True) # blue is not visible without bold
        elif itemType == sequenceParser.eTypeFile:
            filePath = colored.green(os.path.join(filePath + item.getFilename()))
        elif itemType == sequenceParser.eTypeSequence:
            filePath = colored.magenta(os.path.join(filePath + item.getFilename()), bold=True) # magenta is not visible without bold
        elif itemType == sequenceParser.eTypeLink:
            filePath = colored.cyan(os.path.join(filePath + item.getFilename()))
        else:
            filePath += os.path.join(filePath + item.getFilename())
    else:
        filePath += os.path.join(filePath + item.getFilename())
    filePath += ' \t'

    # sam-ls -R / sam-ls -L
    indentTree = ''
    if args.recursive and args.level != 0:
        indentTree += '|  ' * (level - 1)
        indentTree += '|__ '

    # display
    toPrint = detailed + filePath + detailedSequence
    # if first level or no tree formatting
    if level == 0 or args.level == 0:
        puts(toPrint.format())
    else:
        with indent(level, quote=indentTree):
            puts(toPrint.format())


def printItems(items, args, detectionMethod, filters, level=0):
    """
    For each items, check if it should be printed, depending on the command line options.
    """
    for item in sorted(items):
        itemType = item.getType()
        toPrint = True

        # sam-ls -d
        if args.directories and itemType != sequenceParser.eTypeFolder:
            toPrint = False

        # sam-ls -f
        if args.files and itemType != sequenceParser.eTypeFile:
            toPrint = False

        # sam-ls -s
        if args.sequences and itemType != sequenceParser.eTypeSequence:
            toPrint = False

        # print current item
        if toPrint:
            printItem(item, args, level)

        # sam-ls -R
        if args.recursive and itemType == sequenceParser.eTypeFolder:

            # sam-ls -L
            if args.level and args.level <= level:
                continue

            level += 1
            newItems = sequenceParser.browse(os.path.join(item.getFolder(), item.getFilename()), detectionMethod, filters)
            printItems(newItems, args, detectionMethod, filters, level)
            level -= 1


if __name__ == '__main__':

    # Create command-line interface
    parser = argparse.ArgumentParser(
            prog='sam-ls',
            description='''
            List information about the sequences, files and folders.
            List the current directory by default, and only sequences.
            The script option disable color, disable directory printing (in multi-directory case or recursive) and set relative path by default.
            ''',
            )

    parser.add_argument('inputs', nargs='*', action='store', help='list of files/sequences/directories to analyse').completer = common.sequenceParserCompleter

    # Options
    parser.add_argument('-a', '--all', dest='all', action='store_true', help='do not ignore entries starting with .')
    parser.add_argument('-d', '--directories', dest='directories', action='store_true', help='handle directories')
    parser.add_argument('-s', '--sequences', dest='sequences', action='store_true', help='handle sequences')
    parser.add_argument('-f', '--files', dest='files', action='store_true', help='handle files')
    parser.add_argument('-l', '--long-listing', dest='longListing', action='store_true', help='use a long listing format')
    parser.add_argument('-e', '--expression', dest='expression', help='use a specific pattern, ex: *.jpg,*.png').completer = common.sequenceParserCompleter
    parser.add_argument('-R', '--recursive', dest='recursive', action='store_true', help='handle directories and their content recursively')
    parser.add_argument('-L', '--level', dest='level', type=int, help='max display depth of the directory tree (without formatting if 0)')
    parser.add_argument('--absolute-path', dest='absolutePath', action='store_true', help='display the absolute path of each object')
    parser.add_argument('--relative-path', dest='relativePath', action='store_true', help='display the relative path of each object')
    parser.add_argument('--color', dest='color', action='store_true', default=True, help='display the output with colors (True by default)')
    parser.add_argument('--detect-negative', dest='detectNegative', action='store_true', help='detect negative numbers instead of detecting "-" as a non-digit character (False by default)')
    #parser.add_argument('--script', dest='script', help='format the output such as it could be dump in a file and be used as a script')

    # Activate completion
    argcomplete.autocomplete(parser)

    # Parse command-line
    args = parser.parse_args()

    # inputs to scan
    inputs = []
    if args.inputs:
        inputs = args.inputs
    else:
        inputs.append(os.getcwd())

    # sam-ls -a
    detectionMethod = sequenceParser.eDetectionDefault
    if args.all:
        detectionMethod = sequenceParser.eDetectionDefaultWithDotFile

    # sam-ls --detect-negative
    if args.detectNegative:
        detectionMethod = detectionMethod | sequenceParser.eDetectionNegative

    # sam-ls -e
    filters = []
    if args.expression:
        filters.append(args.expression)

    # get list of items for each inputs
    for input in inputs:
        items = []
        try:
            items = sequenceParser.browse(input, detectionMethod, filters)
        except IOError as e:
            # if the given input does not correspond to anything
            if 'No such file or directory' in str(e):
                print e
                continue
            # else it's not a directory: try a new browse with the given input name as filter
            else:
                # new path to browse
                newBrowsePath = os.path.dirname(input)
                if not newBrowsePath:
                    newBrowsePath = '.'
                # new filter
                newFilter = []
                newFilter.extend(filters)
                newFilter.append(os.path.basename(input))
                # new browse
                items += sequenceParser.browse(newBrowsePath, detectionMethod, newFilter)

        printItems(items, args, detectionMethod, filters)
