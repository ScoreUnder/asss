{
  description = "Development flake for ASSS";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
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
        packages = {
          default = self.packages.${system}.asss;
          asss = pkgs.stdenv.mkDerivation {
            pname = "asss";
            version = "1.0";
            src = ./.;
            buildPhase = "make asss";
            installPhase = ''
              mkdir -p "$out"/bin
              cp -r asss "$out"/bin/
            '';
          };
          asss-gui = pkgs.stdenv.mkDerivation {
            pname = "asss-gui";
            version = "1.0";
            src = ./.;
            buildInputs = with pkgs; [
              gtk4
              pkg-config
            ];
            buildPhase = "make asss-gui";
            installPhase = ''
              mkdir -p "$out"/bin
              cp -r asss-gui "$out"/bin/
            '';
          };
        };
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            aflplusplus
          ];
        };
      }
    );
}
