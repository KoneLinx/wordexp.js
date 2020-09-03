/**
 * perform word expansion like a posix-shell
 *
 * @param  { string } input String to expand.
 * @return { string[] } Expanded string.
 */
module.exports = ( input ) =>
{
    return require( './build/Release/wordexp.node' ).wordexp( input );
}