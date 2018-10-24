const path = require('path');
const webpack = require('webpack');
const MinifyPlugin = require("babel-minify-webpack-plugin");
const PROD = JSON.parse(process.env.PROD_ENV || '0');
const minifyOpts = {};
const minigyPluginOpts = {
  test: /\.js($|\?)/i,
};

module.exports = {
  devtool: PROD ? 'none' : 'source-map',
  entry: './source/index.ts',
  mode: PROD ? 'production' : 'development',
  output: {
    filename: 'index.js',
    library: 'Beam',
    libraryTarget: 'umd',
    path: path.resolve(__dirname, 'library/beam'),
    umdNamedDefine: true
  },
  plugins: PROD ? [new MinifyPlugin(minifyOpts, minigyPluginOpts)] : [],
  resolve: {
    extensions: ['.ts', '.js']
  },
  module: {
    rules: [
      {
        test: /\.ts$/,
        loader: 'ts-loader'
      },
      {
        enforce: 'pre',
        test: /\.js$/,
        loader: 'source-map-loader'
      }
    ]
  }
};