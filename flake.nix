
{
  description = "A flake for building the minion constraint solver";

  inputs.nixpkgs.url = "nixpkgs";

  inputs.utils.url = "github:numtide/flake-utils";

  outputs = { self, nixpkgs, utils }: let
    name = "minion";
  in utils.lib.simpleFlake {
    inherit self nixpkgs name;
    systems = utils.lib.defaultSystems;
    overlay = final: prev: {
      minion = with final; rec {
        minion = stdenv.mkDerivation {
          pname = "minion";
          version = "2.0.0-rc1";
          src = self;
          buildInputs = [ python2 ];
          buildPhase = ''
            mkdir build
            cd build
            python2 $src/configure.py
            make minion
          '';
          installPhase = ''
            mkdir -p $out/bin
            cp -a ./minion $out/bin
          '';
        };

        defaultPackage = minion;
      };
    };
  };
}
