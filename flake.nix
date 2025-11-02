{
  description = "PHP development environment for zdump extension";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            php84.unwrapped
            php84Extensions.opcache
            autoconf
            automake
            libtool
            gnumake
            gcc
            pkg-config
          ];

          shellHook = ''
            export PS1="(zdump dev) $PS1"

            echo "PHP zdump development environment"
            echo "=================================="
            echo "PHP version: $(php --version | head -n1)"
            echo ""
            echo "To build the extension:"
            echo "  phpize"
            echo "  ./configure --enable-zdump"
            echo "  make"
            echo ""
            echo "To test (after building):"
            echo "  php -d extension=modules/zdump.so test.php"
            echo "  php -d extension=modules/zdump.so demo.php"
            echo ""
            echo "Extension directory: $(php-config --extension-dir)"
            echo ""
          '';
        };
      }
    );
}
